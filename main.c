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
    ads1115_set_pga(ADS1115_PGA_2_048, &ads1);
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &ads1);
    ads1115_set_operating_mode(ADS1115_MODE_CONTINUOUS, &ads1);

    int16_t adc_values[4];
    uint16_t mux_configs[4] = {
        ADS1115_MUX_SINGLE_0,
        ADS1115_MUX_SINGLE_1,
        ADS1115_MUX_SINGLE_2,
        ADS1115_MUX_SINGLE_3
    };
    
    while (true) {
        // Read all 4 channels
        for (int i = 0; i < 4; i++) {
            ads1115_set_input_mux(mux_configs[i], &ads1);
            ads1115_write_config(&ads1);
            sleep_ms(10); // Wait for channel switch
            ads1115_read_adc(&adc_values[i], &ads1);
        }
        
        // Print all values in one line
        printf("A0:%5d  A1:%5d  A2:%5d  A3:%5d\n", 
               adc_values[0], adc_values[1], adc_values[2], adc_values[3]);

        sleep_ms(10);
    }
}
