#include <stdio.h>
#include <mraa/gpio.h>
#include <unistd.h>

#define SERVO_GPIO_PIN 37

// Function to move the servo to a specific angle (0° to 270°)
void move_servo_soft_pwm(mraa_gpio_context servo_gpio, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;

    // Map angle (0°–270°) to pulse width (1 ms to 2 ms)
    float pulse_ms = 1.0 + (angle / 270.0); // 1ms to 2ms
    int pulse_high_us = (int)(pulse_ms * 1000);
    int period_us = 20000; // 20ms = 50Hz

    // Send multiple pulses to ensure the servo reaches the position
    for (int i = 0; i < 5; ++i) {
        mraa_gpio_write(servo_gpio, 1);
        usleep(pulse_high_us);
        mraa_gpio_write(servo_gpio, 0);
        usleep(period_us - pulse_high_us);
    }
}

int main() {
    mraa_init();

    // Initialize GPIO for servo
    mraa_gpio_context servo_gpio = mraa_gpio_init(SERVO_GPIO_PIN);
    if (servo_gpio == NULL) {
        fprintf(stderr, "Failed to initialize GPIO %d\n", SERVO_GPIO_PIN);
        return 1;
    }
    mraa_gpio_dir(servo_gpio, MRAA_GPIO_OUT);

    printf("Starting Servo Test on GPIO %d\n", SERVO_GPIO_PIN);

    // Sweep angles
    float test_angles[] = {0, 90, 180, 270, 180, 90, 0};
    int num_angles = sizeof(test_angles) / sizeof(test_angles[0]);

    for (int i = 0; i < num_angles; i++) {
        printf("Moving to %.0f°\n", test_angles[i]);
        move_servo_soft_pwm(servo_gpio, test_angles[i]);
        sleep(1); // wait 1 second between positions
    }

    printf("Servo test completed.\n");

    // Cleanup
    mraa_gpio_close(servo_gpio);
    mraa_deinit();
    return 0;
}
