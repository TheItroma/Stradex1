#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "lib/ads1115/ads1115.h"
#include "midi.h"

////////////////////// HARDWARE DEFINITIONS //////////////////////
// I2C Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

// ADS1115 Addresses
#define ADS_1_ADDR 0x48
#define ADS_2_ADDR 0x49

// Pushbutton Pins (PB)
#define NUM_PUSHBUTTONS 4
const int PB[] = {16, 17, 18, 19};

////////////////////// MIDI DEFINITIONS //////////////////////
// Fingerboard configuration
#define NUM_FRETS 16  // Number of frets/semitones available on fingerboard

// Global variables
ads1115_adc_t ads1, ads2;
bool buttons[NUM_PUSHBUTTONS];
int16_t adc_values_1[4];
int16_t adc_values_2[4];
uint16_t mux_configs[4] = {
    ADS1115_MUX_SINGLE_0,
    ADS1115_MUX_SINGLE_1,
    ADS1115_MUX_SINGLE_2,
    ADS1115_MUX_SINGLE_3
};

// MIDI constants
const int open_notes[] = {55, 62, 69, 76};

// Fret boundary positions on soft pot (0-26000 range)
const int fret_boundaries[NUM_FRETS + 1] = {
    0,     // Open string (fret 0)
    9000,  // 1st fret
    13200, // 2nd fret
    14000, // 3rd fret  
    14650, // 4th fret
    15350, // 5th fret
    16150, // 6th fret
    16800, // 7th fret
    17630, // 8th fret
    18550, // 9th fret
    19300, // 10th fret
    20220, // 11th fret
    21100, // 12th fret
    21950, // 13th fret
    22850, // 14th fret
    24000, // 15th fret
    24950  // End boundary
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
        ads1115_read_adc(&adc_values[i], ads);
    }
}

int main()
{
    // Initialize tinyusb
    board_init();
    tusb_init();
    
    ////////////////////// INITIALIZATION //////////////////////

    // Initialize Push Buttons
    init_PB();

    // Initialize I2C
    init_I2C();

    // Initialize ADS1115
    init_ads();

    // Non-blocking timing variables
    uint32_t last_sensor_read = 0;
    const uint32_t sensor_interval_ms = 10; // 10ms interval for sensor reads

    while (true) {
        // Run tinyusb task - this must run as frequently as possible for USB enumeration
        tud_task();

        // Non-blocking sensor reading with timing control
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_sensor_read >= sensor_interval_ms) {
            // Read all sensors
            read_PB();
            read_ads_channels(&ads1, adc_values_1);
            read_ads_channels(&ads2, adc_values_2);

            // Run MIDI note handler
            // It constantly checks for the sensor data and adjusts midi note on and offs accordingly
            midi_note_handler();

            last_sensor_read = current_time;
        }
    }
}
