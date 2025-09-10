#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ads1115.h"

////////////////////// DEFINITIONS //////////////////////
// I2C Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

// ADS1115 Addresses
#define ADS_1_ADDR 0x48
#define ADS_2_ADDR 0x49

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

// Non-blocking ADC reading state variables
typedef struct {
    int current_channel;
    uint32_t last_switch_time;
    bool waiting_for_switch;
} adc_state_t;

adc_state_t adc1_state = {0, 0, false};
adc_state_t adc2_state = {0, 0, false};

// Current midi state variables
int16_t current_note = -1;
int16_t current_pitchbend = 0;
int16_t current_velocity = 0;
bool note_on = false;

// ADS1115 Channel Configurations
uint16_t mux_configs[4] = {
    ADS1115_MUX_SINGLE_0,
    ADS1115_MUX_SINGLE_1,
    ADS1115_MUX_SINGLE_2,
    ADS1115_MUX_SINGLE_3
};

// Fret positions array (17 values from 9000 to 26000)
int16_t fret_positions[17] = {
    9000, 10062, 11125, 12187, 13250, 14312, 15375, 16437, 17500,
    18562, 19625, 20687, 21750, 22812, 23875, 24937, 26000
};

// Base MIDI note values for buttons (G3, D4, A4, E5)
int16_t base_notes[4] = {55, 62, 69, 76}; // G3=55, D4=62, A4=69, E5=76

// Define functions
void serial_debug_print();
void init_I2C();
void init_PB();
void init_ads();
bool read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values, adc_state_t *state);
void read_PB();
void interpret_midi_state();
int16_t get_fret_from_softpot(int16_t softpot_value);

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
    
    while (true) {
        // Read all 4 channels from both ADS units (non-blocking)
        read_ads_channels(&ads1, adc_values_1, &adc1_state);
        read_ads_channels(&ads2, adc_values_2, &adc2_state);
        
        // Read button states
        read_PB();
        
        // Interpret MIDI state from sensor data
        interpret_midi_state();
        
        // Print debug statements
        serial_debug_print();
    }
}

void serial_debug_print() {
    printf("ADS1: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_1[0], adc_values_1[1], adc_values_1[2], adc_values_1[3]);
    printf("ADS2: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_2[0], adc_values_2[1], adc_values_2[2], adc_values_2[3]);
    printf("BTN: %d %d %d %d ", buttons[0], buttons[1], buttons[2], buttons[3]);
    printf("MIDI Note: %d (Note %s)\n", current_note, note_on ? "ON" : "OFF");

    sleep_ms(1);
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

bool read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values, adc_state_t *state) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // If we're waiting for a channel switch to settle
    if (state->waiting_for_switch) {
        if (current_time - state->last_switch_time >= 1) {
            // Channel has settled, read the ADC value
            ads1115_read_adc(&adc_values[state->current_channel], ads);
            state->waiting_for_switch = false;
            state->current_channel++;
            
            // Check if we've read all channels
            if (state->current_channel >= 4) {
                state->current_channel = 0;
                return true; // All channels read
            }
        }
        return false; // Still waiting or not all channels read
    }
    
    // Set up the next channel and start waiting
    ads1115_set_input_mux(mux_configs[state->current_channel], ads);
    ads1115_write_config(ads);
    state->last_switch_time = current_time;
    state->waiting_for_switch = true;
    
    return false; // Channel switch initiated, need to wait
}

// Helper function to determine fret position from softpot value
int16_t get_fret_from_softpot(int16_t softpot_value) {
    // If softpot value is below minimum threshold, no fret is pressed
    if (softpot_value < fret_positions[0]) {
        return 0; // Open string (fret 0)
    }
    
    // Find the closest fret position
    for (int i = 0; i < 16; i++) {
        if (softpot_value >= fret_positions[i] && softpot_value < fret_positions[i + 1]) {
            return i + 1; // Fret number (1-16)
        }
    }
    
    // If above maximum, return highest fret
    return 16;
}

// Main function to interpret sensor data and update MIDI state
void interpret_midi_state() {
    // Reset current note
    current_note = -1;
    note_on = false;
    
    // Check which button is pressed (assuming only one at a time for monophonic)
    int pressed_button = -1;
    for (int i = 0; i < 4; i++) {
        if (buttons[i]) {
            pressed_button = i;
            break; // Take the first pressed button for monophonic behavior
        }
    }
    
    // If no button is pressed, no note should play
    if (pressed_button == -1) {
        return;
    }
    
    // Get the base note for the pressed button
    int16_t base_note = base_notes[pressed_button];
    
    // Determine which ADC channel corresponds to the softpot for this string
    // Assuming adc_values_1[0] is the softpot for now (you may need to adjust this)
    int16_t softpot_value = adc_values_2[0];
    
    // Get fret position from softpot
    int16_t fret = get_fret_from_softpot(softpot_value);
    
    // Calculate final MIDI note (base note + fret offset)
    current_note = base_note + fret;
    note_on = true;
}