#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <mraa.h>
#include <mraa/uart.h>

// === Pin Definitions ===
#define ADC_PIN        6
#define RELAY_PIN      14
#define SERVO_GPIO_PIN 37

// === AHT25 I2C Definitions ===
#define AHT25_I2C_BUS  1
#define AHT25_ADDR     0x38

// === Global Contexts ===
mraa_gpio_context relay_gpio;
mraa_gpio_context servo_gpio;
mraa_aio_context adc_aio;
mraa_i2c_context i2c;

// === Function Prototypes ===
void init_peripherals();
void aht25_init();
int aht25_read(float* temperature, float* humidity);
void move_servo_soft_pwm(float angle);
void control_water_level(uint16_t* level);
void cleanup();

void init_peripherals() {
    mraa_init();

    // Relay
    relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

    // Servo
    servo_gpio = mraa_gpio_init(SERVO_GPIO_PIN);
    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    // ADC
    adc_aio = mraa_aio_init(ADC_PIN);

    // I2C (AHT25)
    i2c = mraa_i2c_init(AHT25_I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init();

    if (!relay_gpio || !adc_aio || !i2c || !servo_gpio) {
        fprintf(stderr, "Peripheral initialization failed\n");
        exit(1);
    }
}

void aht25_init() {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    mraa_i2c_write(i2c, (const char*)init_cmd, 3);
    usleep(10000);
}

int aht25_read(float* temperature, float* humidity) {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    mraa_i2c_write(i2c, (char*)cmd, 3);
    usleep(80000);

    uint8_t data[6];
    if (mraa_i2c_read(i2c, (char*)data, 6) != 6) return -1;
    if ((data[0] & 0x80) != 0) return -2;

    uint32_t raw_h = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] & 0xF0) >> 4);
    uint32_t raw_t = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    *humidity = ((float)raw_h * 100) / 1048576.0;
    *temperature = ((float)raw_t * 200 / 1048576.0) - 50;

    return 0;
}

void move_servo_soft_pwm(float angle) {
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;

    float pulse_ms = 1.0 + (angle / 270.0);
    int pulse_high_us = (int)(pulse_ms * 1000);
    int period_us = 20000;

    for (int i = 0; i < 5; ++i) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_high_us);
        mraa_gpio_write(servo_gpio, 0);
        usleep(period_us - pulse_high_us);
    }
}

void control_water_level(uint16_t* level) {
    *level = mraa_aio_read(adc_aio);
    printf("Water Level: %u\n", *level);

    if (*level > 700) {
        mraa_gpio_write(relay_gpio, 1);
        printf("Low water level! Pump ON\n");
    } else {
        mraa_gpio_write(relay_gpio, 0);
        printf("Water level OK. Pump OFF\n");
    }
}

void cleanup() {
    mraa_gpio_close(relay_gpio);
    mraa_gpio_close(servo_gpio);
    mraa_aio_close(adc_aio);
    mraa_i2c_stop(i2c);
    mraa_deinit();
}

int main() {
    init_peripherals();

    while (1) {
        float temp = 0.0, hum = 0.0;
        uint16_t level = 0;

        if (aht25_read(&temp, &hum) == 0) {
            printf("Temp: %.2f °C | Hum: %.2f %%\n", temp, hum);
            if (temp >= 30.0) {
                move_servo_soft_pwm(90.0);
                printf("Servo ON\n");
            } else {
                move_servo_soft_pwm(0.0);
                printf("Servo OFF\n");
            }
        } else {
            printf("Sensor error!\n");
        }

        control_water_level(&level);

        sleep(1);
    }

    cleanup();
    return 0;
}
