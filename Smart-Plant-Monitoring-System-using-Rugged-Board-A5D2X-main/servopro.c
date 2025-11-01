#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mraa.h>

#define SERVO_PIN 37
#define I2C_BUS 1
#define AHT25_ADDR 0x38

mraa_gpio_context servo_gpio;
mraa_i2c_context i2c;

void aht25_init() {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    mraa_i2c_write(i2c, (char*)init_cmd, 3);
    usleep(10000);
}

int read_temp(float* temperature) {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    mraa_i2c_write(i2c, (char*)cmd, 3);
    usleep(80000);

    uint8_t data[6];
    if (mraa_i2c_read(i2c, (char*)data, 6) != 6) return -1;
    if ((data[0] & 0x80) != 0) return -2;

    uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    *temperature = ((float)raw_temp * 200 / 1048576.0) - 50;
    return 0;
}

void move_servo(float angle) {
    float pulse_ms = 1.0 + (angle / 270.0);  // maps angle to 1–2ms
    int pulse_us = (int)(pulse_ms * 1000);
    int period_us = 20000;

    for (int i = 0; i < 5; ++i) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_us);
        mraa_gpio_write(servo_gpio, 0);
        usleep(period_us - pulse_us);
    }
}

int main() {
    mraa_init();

    servo_gpio = mraa_gpio_init(SERVO_PIN);
    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    i2c = mraa_i2c_init(I2C_BUS);
    mraa_i2c_address(i2c, AHT25_ADDR);
    aht25_init();

    while (1) {
        float temp;
        if (read_temp(&temp) == 0) {
            printf("Temperature: %.2f °C\n", temp);

            if (temp >= 30.0) {
                move_servo(90);  // ON position
                printf("Servo ON\n");
            } else {
                move_servo(0);   // OFF position
                printf("Servo OFF\n");
            }
        } else {
            printf("Sensor error.\n");
        }

        sleep(1);
    }

    mraa_gpio_close(servo_gpio);
    mraa_i2c_stop(i2c);
    mraa_deinit();
    return 0;
}
