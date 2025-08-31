#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "ads1115.h"

////////////////////// DEFINITIONS //////////////////////
// I2C Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

// ADS1115 Addresses
#define ADS_1_ADDR 0x48
#define ADS_2_ADDR 0x49

// Square Wave Generator
#define SQUARE_WAVE_PIN 26

ads1115_adc_t ads1, ads2;

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

// Sensor value storage variable arrays
bool buttons[NUM_PUSHBUTTONS];
int16_t adc_values_1[4];
int16_t adc_values_2[4];

// Square wave variables
static uint32_t square_wave_frequency = 1000; // Default 1kHz
static absolute_time_t next_toggle_time;
static bool pin_state = false;

// ADS1115 Channel Configurations
uint16_t mux_configs[4] = {
    ADS1115_MUX_SINGLE_0,
    ADS1115_MUX_SINGLE_1,
    ADS1115_MUX_SINGLE_2,
    ADS1115_MUX_SINGLE_3
};

// Define functions
void serial_debug_print();
void init_I2C();
void init_PB();
void init_ads();
void init_square_wave();
void set_square_wave_frequency(uint32_t frequency);
void generate_square_wave();
void read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values);
void read_PB();

int main()
{
    ////////////////////// INITIALIZATION //////////////////////
    stdio_init_all();

    // Initialize Push Buttons
    init_PB();

    // Initialize I2C
    init_I2C();

    // Initialize ADS1115
    // Edit contents of function to fiddle with ADS1115 settings
    init_ads();
    
    // Initialize square wave generator
    init_square_wave();
    
    printf("Starting square wave generator on GPIO %d at %lu Hz\n", SQUARE_WAVE_PIN, square_wave_frequency);
    
    while (true) {
        // Generate square wave (non-blocking)
        generate_square_wave();
        
        // Print status periodically
        static uint32_t last_print = 0;
        static uint32_t loop_count = 0;
        loop_count++;
        
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_print >= 1000) { // Print every second
            printf("Status: %lu Hz on GPIO %d, loops/sec: %lu\n", square_wave_frequency, SQUARE_WAVE_PIN, loop_count);
            last_print = now;
            loop_count = 0;
        }
    }
}

void serial_debug_print() {
    printf("ADS1: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_1[0], adc_values_1[1], adc_values_1[2], adc_values_1[3]);
    printf("ADS2: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_2[0], adc_values_2[1], adc_values_2[2], adc_values_2[3]);
    printf("BTN: %d %d %d %d\n", buttons[0], buttons[1], buttons[2], buttons[3]);
}

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

void init_square_wave() {
    printf("Initializing GPIO %d for square wave output...\n", SQUARE_WAVE_PIN);
    gpio_init(SQUARE_WAVE_PIN);
    gpio_set_dir(SQUARE_WAVE_PIN, GPIO_OUT);
    gpio_put(SQUARE_WAVE_PIN, false);
    pin_state = false;
    
    // Calculate initial toggle time
    uint32_t half_period_us = 500000 / square_wave_frequency; // Half period in microseconds
    next_toggle_time = make_timeout_time_us(half_period_us);
    printf("Square wave initialized: %lu Hz, half period = %lu us\n", square_wave_frequency, half_period_us);
}

void set_square_wave_frequency(uint32_t frequency) {
    if (frequency > 0 && frequency <= 100000) { // Limit to reasonable range
        square_wave_frequency = frequency;
    }
}

void generate_square_wave() {
    static uint32_t toggle_count = 0;
    
    if (absolute_time_diff_us(get_absolute_time(), next_toggle_time) >= 0) {
        // Time to toggle the pin
        pin_state = !pin_state;
        gpio_put(SQUARE_WAVE_PIN, pin_state);
        toggle_count++;
        
        // Print debug info for first few toggles
        if (toggle_count <= 10) {
            printf("Toggle %lu: GPIO %d = %d\n", toggle_count, SQUARE_WAVE_PIN, pin_state);
        }
        
        // Calculate next toggle time
        uint32_t half_period_us = 500000 / square_wave_frequency; // Half period in microseconds
        next_toggle_time = make_timeout_time_us(half_period_us);
    }
}

void read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values) {
    for (int i = 0; i < 4; i++) {
        ads1115_set_input_mux(mux_configs[i], ads);
        ads1115_write_config(ads);
        sleep_ms(10); // Wait for channel switch
        ads1115_read_adc(&adc_values[i], ads);
    }
}