#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <mraa.h> 
#include <mraa/uart.h> 
 
#define UART_DEVICE "/dev/ttyS3"  // Replace with the appropriate UART
 
 
#define LED_PIN1  61  // LED connected to GPIO 61 
#define LED_PIN2  62  // LED connected to GPIO 62 
#define LED_PIN3  63  // LED connected to GPIO 63 
 
void WE10_Init (mraa_uart_context uart) 
{ 
    const char* message; 
    char buffer[256]={0}; 
    int bytes_read; 
    
    /********* CMD+RESET **********/ 
    // Send data over UART 
    message = "CMD+RESET\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    // Read data from UART 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        sleep(1); 
        
        /*********  CMD+WIFIMODE=1  **********/ 
        // Send data over UART 
    message = "CMD+WIFIMODE=1\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer));
        if (bytes_read > 0) {
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
        /********* CMD+CONTOAP=SSID,PASSWD **********/ 
        // Send data over UART 
    message = "CMD+CONTOAP=\"Aayu\",\"byee1234\"\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    sleep(5); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
        /********* CMD?WIFI**/ 
        // Send data over UART 
    message = "CMD?WIFI\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
        
} 
 
 
void MQTT_Init(mraa_uart_context uart) 
{ 
    const char* message; 
    //const char * buffer; 
    char buffer[256]={0}; 
    int bytes_read; 
    
    /*CMD+MQTTNETCFG **********/ 
    // Send data over UART
     message = "CMD+MQTTNETCFG=dev.rightech.io,1883\r\n";
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
    
    /*CMD+MQTTCONCFG **********/ 
    // Send data over UART 
    message = "CMD+MQTTCONCFG=3,mqtt-aayushisamantsinghar21-sbfpla,,,,,,,,,\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
    
    /*CMD+MQTTSTART **********/ 
    // Send data over UART 
    message = "CMD+MQTTSTART=1\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
    
    /*CMD+MQTTSUB *********/ 
    // Send data over UART 
    message = "CMD+MQTTSUB=base/relay/led1\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART
     sleep(1);
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
        // Send data over UART 
    message = "CMD+MQTTSUB=base/relay/led2\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
        // Send data over UART 
    message = "CMD+MQTTSUB=base/relay/led3\r\n"; 
    mraa_uart_write(uart, message, strlen(message)); 
    usleep(2000); 
    // Read data from UART 
        sleep(1); 
        bytes_read = mraa_uart_read(uart, buffer, sizeof(buffer)); 
        if (bytes_read > 0) { 
        printf("Received data: %.*s\n", bytes_read, buffer); 
        } 
        
  
} 
 
 
 
int main() { 
    
    char buffer[128]; 
    int bytes_read; 
    mraa_init(); 
    mraa_gpio_context leds[3];
    leds[0] = mraa_gpio_init(LED_PIN1);
    leds[1] = mraa_gpio_init(LED_PIN2); 
    leds[2] = mraa_gpio_init(LED_PIN3); 
 
    for (int i = 0; i < 3; i++) { 
        mraa_gpio_dir(leds[i], MRAA_GPIO_OUT); 
    } 
    
    for (int i = 0; i < 3; i++) { 
        mraa_gpio_write(leds[i], 1); 
    } 
    
    // Initialize the UART 
    mraa_uart_context uart = mraa_uart_init_raw(UART_DEVICE); 
    if (uart == NULL) { 
        fprintf(stderr, "Failed to initialize UART\n"); 
        return 1; 
    } 
 
    // Set the baud rate and other settings 
    if (mraa_uart_set_baudrate(uart, 38400) != MRAA_SUCCESS) { 
        fprintf(stderr, "Failed to set baud rate\n"); 
        return 1; 
    } 
 
    if (mraa_uart_set_mode(uart, 8, MRAA_UART_PARITY_NONE, 1) != 
MRAA_SUCCESS) { 
        fprintf(stderr, "Failed to set UART mode\n"); 
        return 1; 
    } 
    
    
    WE10_Init(uart); 
    MQTT_Init(uart); 
    
    while(1) 
    { 
    bytes_read = mraa_uart_read(uart, buffer,strlen(buffer)); 
    usleep(10000); 
    //for(int i=0; i<3; i++)
     //leds_control(leds, buffer);
    int i = 0; 
    while (buffer[i] != '\0') 
    { 
        if (buffer[i] == 'N') 
        { 
            printf("Received data: %.*s\n", bytes_read, buffer); 
            printf("inside led1\n"); 
                mraa_gpio_write(leds[0], 0); 
                } 
        else if (buffer[i] == 'F') 
        {     
                printf("Received data: %.*s\n", bytes_read, buffer); 
                printf("inside led1\n"); 
                mraa_gpio_write(leds[0], 1); 
        } 
        else if (buffer[i] == 'L') 
        { 
            printf("Received data: %.*s\n", bytes_read, buffer); 
                printf("Inside led2\n"); 
                mraa_gpio_write(leds[1], 0); 
        } 
        
        else if (buffer[i] == 'M') 
        { 
            printf("Received data: %.*s\n", bytes_read, buffer); 
                printf("Inside led2\n"); 
                mraa_gpio_write(leds[1], 1); 
        } 
        else if (buffer[i] == 'J') 
        { 
            printf("Received data: %.*s\n", bytes_read, buffer); 
            printf("Inside led3\n"); 
                mraa_gpio_write(leds[2], 0); 
        } 
        else if (buffer[i] == 'K') 
        { 
            printf("Received data: %.*s\n", bytes_read, buffer); 
            printf("Inside led3\n"); 
                mraa_gpio_write(leds[2], 1);
       }
	i++; 
    } 
    } 
    // Cleanup and exit 
     mraa_uart_stop(uart); 
     mraa_deinit(); 
     return 0;
}
