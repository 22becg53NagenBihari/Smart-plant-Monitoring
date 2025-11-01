#include <stdio.h>
#include <mraa/gpio.h>
#include <unistd.h>

// Define GPIO pin for servo signal
#define SERVO_GPIO_PIN 37

// Move servo to a specific angle (0 to 180 degrees)
void move_servo_soft_pwm(mraa_gpio_context servo_gpio, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    float pulse_ms = 1.0 + (angle / 180.0); // Maps 0-180° to 1ms–2ms
    int pulse_high_us = (int)(pulse_ms * 1000); // Convert to microseconds
    int period_us = 20000; // 20 ms = 50 Hz

    // Send 5 pulses for stable position
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

    // Example: Move to 0°, 90°, and 180°
    printf("Moving to 0 degrees...\n");
    move_servo_soft_pwm(servo_gpio, 0);
    sleep(1);

    printf("Moving to 90 degrees...\n");
    move_servo_soft_pwm(servo_gpio, 90);
    sleep(1);

    printf("Moving to 180 degrees...\n");
    move_servo_soft_pwm(servo_gpio, 180);
    sleep(1);

    mraa_gpio_close(servo_gpio);
    mraa_deinit();
    return 0;
}
