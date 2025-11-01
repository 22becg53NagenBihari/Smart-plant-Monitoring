#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <mraa.h>
#include <mraa/uart.h>

// === Pin Definitions ===
#define ADC_PIN     6
#define RELAY_PIN   14
#define LED_WATER   61
#define LED1        61
#define LED2        62
#define LED3        63

#define UART_DEVICE "/dev/ttyS3"
#define AHT25_I2C_BUS 1
#define AHT25_ADDR    0x38

// === Global Contexts ===
mraa_gpio_context led_gpio, relay_gpio, leds[3];
mraa_aio_context adc_aio;
mraa_i2c_context i2c;
mraa_uart_context uart;

// === Function Prototypes ===
void init_peripherals();
void aht25_init_sensor();
int aht25_read_sensor(float* temperature, float* humidity);
void control_water_level();
void mqtt_publish_sensor_data(float temp, float hum, uint16_t water);
void mqtt_led_control();
void WE10_Init();
void MQTT_Init();
void cleanup();

void init_peripherals() {
    mraa_init();

    // GPIOs
    led_gpio = mraa_gpio_init(LED_WATER);
    relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_dir(led_gpio, MRAA_GPIO_OUT);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

    leds[0] = mraa_gpio_init(LED1);
    leds[1] = mraa_gpio_init(LED2);
    leds[2] = mraa_gpio_init(LED3);
    for (int i = 0; i < 3; i++) mraa_gpio_dir(leds[i], MRAA_GPIO_OUT);

    // ADC
    adc_aio = mraa_aio_init(ADC_PIN);

    // I2C (AHT25)
    i2c = mraa_i2c_init(AHT25_I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init_sensor();

    // UART
    uart = mraa_uart_init_raw(UART_DEVICE);
    mraa_uart_set_baudrate(uart, 38400);
    mraa_uart_set_mode(uart, 8, MRAA_UART_PARITY_NONE, 1);

    WE10_Init();
    MQTT_Init();
}

void aht25_init_sensor() {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    mraa_i2c_write(i2c, (const char*)init_cmd, 3);
    usleep(10000);
}

int aht25_read_sensor(float* temperature, float* humidity) {
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};
    mraa_i2c_write(i2c, (const char*)trigger_cmd, 3);
    usleep(80000);

    uint8_t buffer[6];
    if (mraa_i2c_read(i2c, (char*)buffer, 6) != 6) return -1;
    if ((buffer[0] & 0x80) != 0) return -2;

    uint32_t raw_humidity = ((uint32_t)(buffer[1]) << 12) | ((uint32_t)(buffer[2]) << 4) | ((buffer[3] & 0xF0) >> 4);
    uint32_t raw_temp = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)(buffer[4]) << 8) | buffer[5];

    *humidity = ((float)raw_humidity * 100) / 1048576.0;
    *temperature = ((float)raw_temp * 200 / 1048576.0) - 50;

    return 0;
}

void control_water_level() {
    uint16_t adc_value = mraa_aio_read(adc_aio);
    printf("ADC Value (Water Level): %u\n", adc_value);

    if (adc_value > 700) {
        mraa_gpio_write(led_gpio, 0);
        mraa_gpio_write(relay_gpio, 0);
        printf("Insufficient water! Water Pump ON\n");
    } else {
        mraa_gpio_write(led_gpio, 1);
        mraa_gpio_write(relay_gpio, 1);
        printf("Sufficient water. Water Pump OFF\n");
    }
}

void mqtt_publish_sensor_data(float temp, float hum, uint16_t water) {
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "CMD+MQTTPUB=base/relay/temp,%.2f\r\n", temp);
    mraa_uart_write(uart, cmd, strlen(cmd));
    usleep(100000);

    snprintf(cmd, sizeof(cmd), "CMD+MQTTPUB=base/relay/hum,%.2f\r\n", hum);
    mraa_uart_write(uart, cmd, strlen(cmd));
    usleep(100000);

    snprintf(cmd, sizeof(cmd), "CMD+MQTTPUB=base/relay/water,%d\r\n", (water > 700) ? 1 : 0);
    mraa_uart_write(uart, cmd, strlen(cmd));
    usleep(100000);
}

void mqtt_led_control() {
    char buffer[128] = {0};
    int bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer));
    for (int i = 0; i < bytes_read; i++) {
        switch (buffer[i]) {
            case 'N': mraa_gpio_write(leds[0], 0); break;
            case 'F': mraa_gpio_write(leds[0], 1); break;
            case 'L': mraa_gpio_write(leds[1], 0); break;
            case 'M': mraa_gpio_write(leds[1], 1); break;
            case 'J': mraa_gpio_write(leds[2], 0); break;
            case 'K': mraa_gpio_write(leds[2], 1); break;
        }
    }
}

void WE10_Init() {
    const char* cmds[] = {
        "CMD+RESET\r\n",
        "CMD+WIFIMODE=1\r\n",
        "CMD+CONTOAP=\"Aayu\",\"byee1234\"\r\n",
        "CMD?WIFI\r\n"
    };
    char buffer[256];
    for (int i = 0; i < 4; i++) {
        mraa_uart_write(uart, cmds[i], strlen(cmds[i]));
        sleep(1);
        int bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer));
        if (bytes_read > 0)
            printf("%.*s\n", bytes_read, buffer);
    }
}

void MQTT_Init() {
    const char* cmds[] = {
        "CMD+MQTTNETCFG=dev.rightech.io,1883\r\n",
        "CMD+MQTTCONCFG=3,mqtt-aayushisamantsinghar21-sbfpla,,,,,,,,,\r\n",
        "CMD+MQTTSTART=1\r\n",
        "CMD+MQTTSUB=base/relay/led1\r\n",
        "CMD+MQTTSUB=base/relay/led2\r\n",
        "CMD+MQTTSUB=base/relay/led3\r\n"
    };
    char buffer[256];
    for (int i = 0; i < 6; i++) {
        mraa_uart_write(uart, cmds[i], strlen(cmds[i]));
        sleep(1);
        int bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer));
        if (bytes_read > 0)
            printf("%.*s\n", bytes_read, buffer);
    }
}

void cleanup() {
    for (int i = 0; i < 3; i++) mraa_gpio_close(leds[i]);
    mraa_gpio_close(led_gpio);
    mraa_gpio_close(relay_gpio);
    mraa_aio_close(adc_aio);
    mraa_i2c_stop(i2c);
    mraa_uart_stop(uart);
    mraa_deinit();
}

int main() {
    init_peripherals();

    while (1) {
        control_water_level();
        float temp = 0.0, hum = 0.0;
        uint16_t water = mraa_aio_read(adc_aio);

        if (aht25_read_sensor(&temp, &hum) == 0) {
            printf("Temp: %.2f C, Hum: %.2f %%\n", temp, hum);
            mqtt_publish_sensor_data(temp, hum, water);
        } else {
            printf("Sensor read failed.\n");
        }

        mqtt_led_control();
        sleep(1);
    }

    cleanup();
    return 0;
}

