#include<stdio.h> 
#include<mraa.h>
#define RELAY_PIN 14
#define ADC_PIN 6 
#define LED_PIN 61 
int main() 
{ 
	mraa_init(); 
	mraa_gpio_context led_gpio=mraa_gpio_init (LED_PIN); 
	mraa_aio_context adc_aio= mraa_aio_init (ADC_PIN);
        mraa_gpio_context relay_gpio = mraa_gpio_init (RELAY_PIN);	
	if (led_gpio==NULL || adc_aio==NULL)
       	{ 
	       	fprintf (stderr,"Failed to initialize AIO\n"); 
		return 1;
	} 
	mraa_gpio_dir (led_gpio, MRAA_GPIO_OUT);
        mraa_gpio_dir (relay_gpio, MRAA_GPIO_OUT);	
	while (1) 
	{ 
		uint16_t  value= mraa_aio_read(adc_aio); 
		printf ("ADC value: %u\n",value); 
		if(value>700) 
		{ 
			mraa_gpio_write(led_gpio,0); //Turn on the led 
			mraa_gpio_write(relay_gpio,0);
		       	printf ("\nInsufficient water!!!Turn on Water Pump\n"); 
		}
	       	else 
		{ 
		       	mraa_gpio_write(led_gpio,1); //Turn off the led
		        mraa_gpio_write(relay_gpio,1);	
		       	printf ("\nSufficient water...Turn off Water Pump\n"); 
		} 
	} 
       	usleep (500000);   
	mraa_gpio_close(led_gpio); 
	mraa_aio_close(adc_aio); 
	mraa_gpio_close(relay_gpio);
	mraa_deinit(); 
	return 0;
}
