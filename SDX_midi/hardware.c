#include "hardware.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

// Global variable definitions
ads1115_adc_t ads1, ads2;
bool buttons[NUM_PUSHBUTTONS];
int16_t adc_values_1[4];
int16_t adc_values_2[4];
bool device_mounted = false;

// Hardware configuration arrays
const int PB[] = {16, 17, 18, 19};
uint16_t mux_configs[4] = {
    ADS1115_MUX_SINGLE_0,
    ADS1115_MUX_SINGLE_1,
    ADS1115_MUX_SINGLE_2,
    ADS1115_MUX_SINGLE_3
};

////////////////////// HARDWARE FUNCTIONS //////////////////////
void init_I2C() {
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void init_PB() {
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
        gpio_init(PB[i]);
        gpio_set_dir(PB[i], GPIO_IN);
    }
}

void init_ads() {
    ads1115_init(I2C_PORT, ADS_1_ADDR, &ads1);
    ads1115_init(I2C_PORT, ADS_2_ADDR, &ads2);

    ads1115_set_pga(ADS1115_PGA_4_096, &ads1);
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &ads1);
    ads1115_set_operating_mode(ADS1115_MODE_CONTINUOUS, &ads1);
    ads1115_set_pga(ADS1115_PGA_4_096, &ads2);
    ads1115_set_data_rate(ADS1115_RATE_860_SPS, &ads2);
    ads1115_set_operating_mode(ADS1115_MODE_CONTINUOUS, &ads2);
}

void read_PB() {
    for (int i = 0; i < 4; i++) {
        buttons[i] = gpio_get(PB[i]);
    }
}

void read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values) {
    for (int i = 0; i < 4; i++) {
        ads1115_set_input_mux(mux_configs[i], ads);
        ads1115_write_config(ads);
        // sleep_ms(10); // Wait for channel switch
        ads1115_read_adc(&adc_values[i], ads);
    }
}
