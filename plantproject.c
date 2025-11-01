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

// === AHT25 I2C Definitions ===
#define AHT25_I2C_BUS 1
#define AHT25_ADDR    0x38

// === UART Definition ===
#define UART_DEVICE "/dev/ttyS3"

// === Global Contexts ===
mraa_gpio_context relay_gpio;
mraa_aio_context adc_aio;
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
void cleanup();

void init_peripherals() {
    mraa_init();

    // Relay
    relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

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

    // Error handling
    if (!relay_gpio || !adc_aio || !i2c || !uart) {
        fprintf(stderr, "Peripheral initialization failed\n");
        exit(1);
    }
}

void control_water_level(uint16_t* water_level) {
    *water_level = mraa_aio_read(adc_aio);
    printf("Water Level (ADC): %u\n", *water_level);

    if (*water_level > 700) {
        mraa_gpio_write(relay_gpio, 0); // Relay ON
        printf("Low water level! Pump ON\n");
    } else {
        mraa_gpio_write(relay_gpio, 1); // Relay OFF
        printf("Water level sufficient. Pump OFF\n");
    }
}

void aht25_init_sensor() {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    mraa_i2c_write(i2c, (const char*)init_cmd, 3);
    usleep(10000); // 10 ms
}

int aht25_read_sensor(float* temperature, float* humidity) {
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};
    mraa_i2c_write(i2c, (const char*)trigger_cmd, 3);
    usleep(80000); // 80 ms

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
    char buffer[256] = {0};

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
    char buffer[256] = {0};

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
    usleep(200000); // 200 ms
}

void cleanup() {
    mraa_gpio_close(relay_gpio);
    mraa_aio_close(adc_aio);
    mraa_i2c_stop(i2c);
    mraa_uart_stop(uart);
    mraa_deinit();
}

