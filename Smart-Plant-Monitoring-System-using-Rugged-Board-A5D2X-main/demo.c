#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <mraa.h>
#include <mraa/uart.h>

// === Pin Definitions ===
#define ADC_PIN       6     // Water level sensor
#define RELAY_PIN     14    // Water pump relay
#define LDR_PIN       7     // LDR ADC
#define SERVO_PIN     15    // Servo GPIO

// === AHT25 I2C Definitions ===
#define AHT25_I2C_BUS 1
#define AHT25_ADDR    0x38

// === UART Definition ===
#define UART_DEVICE "/dev/ttyS3"

// === Global Contexts ===
mraa_gpio_context relay_gpio;
mraa_gpio_context servo_gpio;
mraa_aio_context adc_aio;
mraa_aio_context ldr_aio;
mraa_i2c_context i2c;
mraa_uart_context uart;

// === Function Prototypes ===
void init_peripherals();
void control_water_level(uint16_t* water_level);
void aht25_init_sensor();
int aht25_read_sensor(float* temperature, float* humidity);
void WE10_Init();
void MQTT_Init();
void MQTT_Publish(const char* topic, const char* message);
void process_rightec_commands();
void servo_write(int degree);
void cleanup();

void init_peripherals() {
    mraa_init();

    // Relay
    relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

    // ADC (Water sensor)
    adc_aio = mraa_aio_init(ADC_PIN);
    ldr_aio = mraa_aio_init(LDR_PIN);

    // I2C (AHT25)
    i2c = mraa_i2c_init(AHT25_I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init_sensor();

    // Servo GPIO
    servo_gpio = mraa_gpio_init(SERVO_PIN);
    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    // UART
    uart = mraa_uart_init_raw(UART_DEVICE);
    mraa_uart_set_baudrate(uart, 38400);
    mraa_uart_set_mode(uart, 8, MRAA_UART_PARITY_NONE, 1);

    // Error handling
    if (!relay_gpio || !adc_aio || !ldr_aio || !servo_gpio || !i2c || !uart) {
        fprintf(stderr, "Peripheral initialization failed\n");
        exit(1);
    }
}

void control_water_level(uint16_t* water_level) {
    *water_level = mraa_aio_read(adc_aio);
    printf("Water Level (ADC): %u\n", *water_level);
    mraa_gpio_write(relay_gpio, *water_level > 700 ? 0 : 1);
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

void WE10_Init() {
    const char* cmd;

    cmd = "CMD+RESET\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);

    cmd = "CMD+WIFIMODE=1\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);

    cmd = "CMD+CONTOAP=\"Aayu\",\"byee1234\"\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(5);

    cmd = "CMD?WIFI\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);
}

void MQTT_Init() {
    const char* cmd;

    cmd = "CMD+MQTTNETCFG=dev.rightech.io,1883\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);

    cmd = "CMD+MQTTCONCFG=3,mqtt-aayushisamantsinghar21-hakgvq,,,,,,,,,\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);

    cmd = "CMD+MQTTSTART=1\r\n";
    mraa_uart_write(uart, cmd, strlen(cmd));
    sleep(1);
}

void MQTT_Publish(const char* topic, const char* message) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "CMD+MQTTPUB=%s,%s\r\n", topic, message);
    mraa_uart_write(uart, cmd, strlen(cmd));
    usleep(200000);
}

void process_rightec_commands() {
    char buffer[128] = {0};
    int len = mraa_uart_read(uart, buffer, sizeof(buffer));
    if (len > 0) {
        if (strstr(buffer, "relay=1")) mraa_gpio_write(relay_gpio, 0);
        else if (strstr(buffer, "relay=0")) mraa_gpio_write(relay_gpio, 1);

        if (strstr(buffer, "servo=1")) servo_write(90);
        else if (strstr(buffer, "servo=0")) servo_write(0);
    }
}

void servo_write(int degree) {
    int pulse_width = (degree * 2000 / 180) + 500; // 0.5ms to 2.5ms
    for (int i = 0; i < 10; i++) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_width);
        mraa_gpio_write(servo_gpio, 0);
        usleep(20000 - pulse_width);
    }
}

void cleanup() {
    mraa_gpio_close(relay_gpio);
    mraa_gpio_close(servo_gpio);
    mraa_aio_close(adc_aio);
    mraa_aio_close(ldr_aio);
    mraa_i2c_stop(i2c);
    mraa_uart_stop(uart);
    mraa_deinit();
}

int main() {
    init_peripherals();
    WE10_Init();
    MQTT_Init();

    while (1) {
        float temp = 0.0, hum = 0.0;
        uint16_t water_level = 0;
        uint16_t ldr = mraa_aio_read(ldr_aio);

        if (aht25_read_sensor(&temp, &hum) == 0) {
            char temp_str[32];
            char hum_str[32];
            snprintf(temp_str, sizeof(temp_str), "%.2f", temp);
            snprintf(hum_str, sizeof(hum_str), "%.2f", hum);

            MQTT_Publish("base/temp", temp_str);
            MQTT_Publish("base/hum", hum_str);
        }

        control_water_level(&water_level);
        char water_str[16];
        snprintf(water_str, sizeof(water_str), "%u", water_level);
        MQTT_Publish("base/water", water_str);

        char ldr_str[16];
        snprintf(ldr_str, sizeof(ldr_str), "%u", ldr);
        MQTT_Publish("base/ldr", ldr_str);

        process_rightec_commands();
        sleep(2);
    }

    cleanup();
    return 0;
}
