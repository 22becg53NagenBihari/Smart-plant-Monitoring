#include <stdio.h>
#include <mraa.h>
#include <unistd.h>  // For usleep

#define ADC_PIN 6 
#define RELAY_PIN 14  // Relay connected to GPIO 14

int main()
{
    mraa_init();

    mraa_gpio_context relay_gpio = mraa_gpio_init(RELAY_PIN);
    mraa_aio_context adc_aio = mraa_aio_init(ADC_PIN);

    if (relay_gpio == NULL || adc_aio == NULL)
    {
        fprintf(stderr, "Failed to initialize GPIO or AIO\n");
        return 1;
    }

    mraa_gpio_dir(relay_gpio, MRAA_GPIO_OUT);

    while (1)
    {
        uint16_t value = mraa_aio_read(adc_aio);
        printf("ADC value: %u\n", value);

        if (value > 700)
        {
            mraa_gpio_write(relay_gpio, 0);  // Turn ON relay (active LOW)
            printf("Insufficient water!!! Turning ON water pump (relay ON)\n");
        }
        else
        {
            mraa_gpio_write(relay_gpio, 1);  // Turn OFF relay
            printf("Sufficient water... Turning OFF water pump (relay OFF)\n");
        }

        usleep(500000);  // Sleep for 500ms
    }

    mraa_gpio_close(relay_gpio);
    mraa_aio_close(adc_aio);
    mraa_deinit();

    return 0;
}
