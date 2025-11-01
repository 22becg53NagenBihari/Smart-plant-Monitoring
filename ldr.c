 #include<stdio.h> 
 #include<mraa.h> 
 #define LDR 12 // LDR CONNECTED TO ANALOG PIN 0 
int main() 
{ 
	mraa_init(); 
 
        mraa_gpio_context led; 
 
        led = mraa_gpio_init(61); // GPIO PIN 61 FOR LED 
        mraa_gpio_dir(led, MRAA_GPIO_OUT); 
 
        mraa_aio_context adc; 
        adc = mraa_aio_init(LDR); 
 
        if(adc == NULL) 
        { 
		fprintf(stderr, "Failed to initialise AIO\n"); 
                return 1; 
        } 
        while(1) 
        { 
                uint16_t value = mraa_aio_read(adc); 
                printf("LDR Value: %u\n",value); 
                if(value > 600) 
                        mraa_gpio_write(led, 0); 
                else 
                        mraa_gpio_write(led, 1); 
 
                usleep(500000); // DELAY FOR 0.5 SECONDS 
        } 
        mraa_aio_close(adc); 
        mraa_deinit(); 

        return 0; 
} 
