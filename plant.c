#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <mraa.h>
#include <string.h>

// === Pin Definitions ===
#define ADC_PIN     6
#define LED_PIN     61
#define RELAY_PIN   14

// === AHT25 I2C Definitions ===
#define AHT25_I2C_BUS 1
#define AHT25_ADDR    0x38

// === Global Contexts ===
mraa_gpio_context led_gpio;
mraa_gpio_context relay_gpio;
mraa_aio_context adc_aio;
mraa_i2c_context i2c;

// === Function Prototypes ===
void init_peripherals();
void control_water_level();
void aht25_init_sensor();
int aht25_read_sensor(float* temperature, float* humidity);
void cleanup();

void init_peripherals() {
    mraa_init();

    // GPIOs
    led_gpio = mraa_gpio_init(LED_PIN);
    relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_dir(led_gpio, MRAA_GPIO_OUT);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

    // ADC
    adc_aio = mraa_aio_init(ADC_PIN);

    // I2C (AHT25)
    i2c = mraa_i2c_init(AHT25_I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init_sensor();

    // Error handling
    if (!led_gpio || !relay_gpio || !adc_aio || !i2c) {
        fprintf(stderr, "Peripheral initialization failed\n");
        exit(1);
    }
}

void control_water_level() {
    uint16_t adc_value = mraa_aio_read(adc_aio);
    printf("ADC Value (Water Level): %u\n", adc_value);

    if (adc_value > 700) {
        mraa_gpio_write(led_gpio, 0);   // LED ON
        mraa_gpio_write(relay_gpio, 0); // Relay ON
        printf("Insufficient water! Water Pump ON\n");
    } else {
        mraa_gpio_write(led_gpio, 1);   // LED OFF
        mraa_gpio_write(relay_gpio, 1); // Relay OFF
        printf("Sufficient water. Water Pump OFF\n");
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

void cleanup() {
    mraa_gpio_close(led_gpio);
    mraa_gpio_close(relay_gpio);
    mraa_aio_close(adc_aio);
    mraa_i2c_stop(i2c);
    mraa_deinit();
}

int main() {
    init_peripherals();

    while (1) {
        control_water_level();

        float temp = 0.0, hum = 0.0;
        if (aht25_read_sensor(&temp, &hum) == 0) {
            printf("Temperature: %.2f °C | Humidity: %.2f %%\n", temp, hum);
        } else {
            printf("Failed to read AHT25 sensor data.\n");
        }

        usleep(500000); // 500 ms
    }

    cleanup(); // Not reached in this loop
    return 0;
}

