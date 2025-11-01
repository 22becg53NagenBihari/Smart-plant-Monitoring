#include <stdio.h>
#include <mraa.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ADC_PIN 6
#define LED_PIN 61
#define RELAY_PIN 14
#define SERVO_GPIO_PIN 37         // Using GPIO pin 37 for software PWM
#define AHT25_I2C_BUS 1
#define AHT25_ADDR 0x38

void aht25_init(mraa_i2c_context i2c) {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    mraa_i2c_write(i2c, (const char*)init_cmd, 3);
    usleep(10000);
}

int aht25_read(mraa_i2c_context i2c, float* temperature, float* humidity) {
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};
    mraa_i2c_write(i2c, (const char*)trigger_cmd, 3);
    usleep(80000);

    uint8_t buffer[6];
    if (mraa_i2c_read(i2c, (char*)buffer, 6) != 6) {
        return -1;
    }

    if ((buffer[0] & 0x80) != 0) {
        return -2;
    }

    uint32_t raw_humidity = ((uint32_t)(buffer[1]) << 12) | ((uint32_t)(buffer[2]) << 4) | ((buffer[3] & 0xF0) >> 4);
    uint32_t raw_temp = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)(buffer[4]) << 8) | buffer[5];

    *humidity = ((float)raw_humidity * 100) / 1048576.0;
    *temperature = ((float)raw_temp * 200 / 1048576.0) - 50;

    return 0;
}

void move_servo_soft_pwm(mraa_gpio_context servo_gpio, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    float pulse_ms = 1.0 + (angle / 180.0); // 1 ms to 2 ms
    int pulse_high_us = (int)(pulse_ms * 1000);
    int pulse_period_us = 20000; // 20 ms period (50 Hz)

    // Send 5 PWM pulses for stable movement
    for (int i = 0; i < 5; ++i) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_high_us);
        mraa_gpio_write(servo_gpio, 0);
        usleep(pulse_period_us - pulse_high_us);
    }
}

int main() {
    mraa_init();

    // GPIO Initialization
    mraa_gpio_context led_gpio = mraa_gpio_init(LED_PIN);
    mraa_gpio_context relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_gpio_context servo_gpio = mraa_gpio_init(SERVO_GPIO_PIN);

    // ADC Initialization
    mraa_aio_context adc_aio = mraa_aio_init(ADC_PIN);

    // I2C Initialization for AHT25
    mraa_i2c_context i2c = mraa_i2c_init(AHT25_I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init(i2c);

    if (led_gpio == NULL || relay_gpio == NULL || adc_aio == NULL || i2c == NULL || servo_gpio == NULL) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    mraa_gpio_dir(led_gpio, MRAA_GPIO_OUT);
    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);
    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    while (1) {
        // ADC Read
        uint16_t adc_value = mraa_aio_read(adc_aio);
        printf("ADC value: %u\n", adc_value);

        if (adc_value > 700) {
            mraa_gpio_write(led_gpio, 0);
            mraa_gpio_write(relay_gpio, 0);
            printf("Insufficient water! Water Pump ON (LED & Relay ON)\n");
        } else {
            mraa_gpio_write(led_gpio, 1);
            mraa_gpio_write(relay_gpio, 1);
            printf("Sufficient water. Water Pump OFF (LED & Relay OFF)\n");
        }

        // Temperature & Humidity Read
        float temperature = 0.0, humidity = 0.0;
        if (aht25_read(i2c, &temperature, &humidity) == 0) {
            printf("Temperature: %.2f °C, Humidity: %.2f %%\n", temperature, humidity);

            if (temperature > 30.0) {
                printf("Temperature exceeds 30°C! Moving servo to 180 degrees.\n");
                move_servo_soft_pwm(servo_gpio, 180.0);
            } else {
                printf("Temperature normal. Moving servo to 0 degrees.\n");
                move_servo_soft_pwm(servo_gpio, 0.0);
            }

        } else {
            printf("Failed to read AHT25 sensor data.\n");
        }

        usleep(500000); // 500ms delay
    }

    // Cleanup
    mraa_gpio_close(led_gpio);
    mraa_gpio_close(relay_gpio);
    mraa_gpio_close(servo_gpio);
    mraa_aio_close(adc_aio);
    mraa_i2c_stop(i2c);
    mraa_deinit();

    return 0;
}
