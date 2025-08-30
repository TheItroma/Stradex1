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

    // Configure ADS1115 for reading all channels
    ads1115_set_pga(ADS1115_PGA_4_096, &ads1);
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &ads1);
    ads1115_set_operating_mode(ADS1115_MODE_CONTINUOUS, &ads1);
    
    ads1115_set_pga(ADS1115_PGA_4_096, &ads2);
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &ads2);
    ads1115_set_operating_mode(ADS1115_MODE_CONTINUOUS, &ads2);

    int16_t adc_values_1[4];
    int16_t adc_values_2[4];
    uint16_t mux_configs[4] = {
        ADS1115_MUX_SINGLE_0,
        ADS1115_MUX_SINGLE_1,
        ADS1115_MUX_SINGLE_2,
        ADS1115_MUX_SINGLE_3
    };
    
    while (true) {
        // Read all 4 channels from ADS1
        for (int i = 0; i < 4; i++) {
            ads1115_set_input_mux(mux_configs[i], &ads1);
            ads1115_write_config(&ads1);
            sleep_ms(10); // Wait for channel switch
            ads1115_read_adc(&adc_values_1[i], &ads1);
        }
        
        // Read all 4 channels from ADS2
        for (int i = 0; i < 4; i++) {
            ads1115_set_input_mux(mux_configs[i], &ads2);
            ads1115_write_config(&ads2);
            sleep_ms(10); // Wait for channel switch
            ads1115_read_adc(&adc_values_2[i], &ads2);
        }
        
        // Read button states
        bool buttons[4];
        for (int i = 0; i < 4; i++) {
            buttons[i] = gpio_get(PB[i]);
        }
        
        // Print all values from both ADS units
        printf("ADS1: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
               adc_values_1[0], adc_values_1[1], adc_values_1[2], adc_values_1[3]);
        printf("ADS2: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
               adc_values_2[0], adc_values_2[1], adc_values_2[2], adc_values_2[3]);
        printf("BTN: %d %d %d %d\n", buttons[0], buttons[1], buttons[2], buttons[3]);

        sleep_ms(1);
    }
}
