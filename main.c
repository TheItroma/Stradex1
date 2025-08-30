#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ads1115.h"

// I2C Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

// ADS1115 Addresses
#define ADS_1_ADDR 0x48
#define ADS_2_ADDR 0x49

// ALRT Pins
#define ADS_1_ALRT 6
#define ADS_2_ALRT 7

// Pushbutton Pins (PB)
const int PB[] = {16, 17, 18, 19};
#define NUM_PUSHBUTTONS (sizeof(PB) / sizeof(PB[0]))
#define PB_1 PB[0]
#define PB_2 PB[1] 
#define PB_3 PB[2]
#define PB_4 PB[3]

int main()
{
    stdio_init_all();

    // PB Init
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
        gpio_init(PB[i]);
        gpio_set_dir(PB[i], GPIO_IN);
    }

    // I2C Init. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // ADS1115 Init
    ads1115_adc_t ads1, ads2;
    ads1115_init(I2C_PORT, ADS_1_ADDR, &ads1);
    ads1115_init(I2C_PORT, ADS_2_ADDR, &ads2);
    
    while (true) {
        // Read ADS1115 #1 (0x48) - all 4 channels
        printf("ADS1: ");
        for (int ch = 0; ch < 4; ch++) {
            ads1115_set_input_mux(ADS1115_MUX_SINGLE_0 + (ch << 12), &ads1);
            uint16_t raw_value;
            ads1115_read_adc(&raw_value, &ads1);
            printf("CH%d:%d ", ch, raw_value);
        }
        
        printf("| ADS2: ");
        // Read ADS1115 #2 (0x49) - all 4 channels
        for (int ch = 0; ch < 4; ch++) {
            ads1115_set_input_mux(ADS1115_MUX_SINGLE_0 + (ch << 12), &ads2);
            uint16_t raw_value;
            ads1115_read_adc(&raw_value, &ads2);
            printf("CH%d:%d ", ch, raw_value);
        }
        
        printf("\n");
        sleep_ms(50); // Slower update rate for readability
    }
}
