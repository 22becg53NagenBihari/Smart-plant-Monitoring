#include <stdio.h>
#include <mraa/gpio.h>
#include <unistd.h>

#define SERVO_GPIO_PIN 37

void move_servo_soft_pwm(mraa_gpio_context servo_gpio, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;

    // Map 0–270° to 1ms–2ms pulse width range
    float pulse_ms = 1.0 + (angle / 270.0);  // 1.0ms to 2.0ms for 270°
    int pulse_high_us = (int)(pulse_ms * 1000);
    int period_us = 20000; // 20ms = 50Hz

    // Send multiple pulses to reach and hold position
    for (int i = 0; i < 5; ++i) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_high_us);
        mraa_gpio_write(servo_gpio, 0);
        usleep(period_us - pulse_high_us);
    }
}

int main() {
    mraa_init();
    mraa_gpio_context servo_gpio = mraa_gpio_init(SERVO_GPIO_PIN);
    if (servo_gpio == NULL) {
        fprintf(stderr, "Failed to initialize GPIO %d\n", SERVO_GPIO_PIN);
        return 1;
    }

    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    printf("Moving to 0°...\n");
    move_servo_soft_pwm(servo_gpio, 0);
    sleep(1);

    printf("Moving to 135°...\n");
    move_servo_soft_pwm(servo_gpio, 135);
    sleep(1);

    printf("Moving to 270°...\n");
    move_servo_soft_pwm(servo_gpio, 270);
    sleep(1);

    mraa_gpio_close(servo_gpio);
    mraa_deinit();
    return 0;
}
