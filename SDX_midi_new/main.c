#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ads1115.h"
#include "tusb.h"

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
int16_t previous_note = -1;
int16_t current_pitchbend = 0;
int16_t current_velocity = 0;
int16_t current_volume = 127;
int16_t previous_volume = 127;
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
    24950,  // End boundary
    26000  // End boundary
};

// Base MIDI note values for buttons (G3, D4, A4, E5)
int16_t base_notes[4] = {55, 62, 69, 76}; // G3=55, D4=62, A4=69, E5=76

// FSR to Volume mapping configuration
#define FSR_MIN_VALUE 1000    // FSR value for maximum volume
#define FSR_MAX_VALUE 22000   // FSR value for minimum volume
#define VOLUME_MAX 127        // Maximum MIDI volume (100%)
#define VOLUME_MIN 13         // Minimum MIDI volume (~10%)

// Define functions
void serial_debug_print();
void init_I2C();
void init_PB();
void init_ads();
bool read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values, adc_state_t *state);
void read_PB();
void interpret_midi_state();
int16_t get_fret_from_softpot(int16_t softpot_value);
void send_note_on(int16_t note);
void send_note_off(int16_t note);
int16_t fsr_to_volume(int16_t fsr_value);
void send_volume_control(int16_t volume);

int main()
{
    ////////////////////// INITIALIZATION //////////////////////
    stdio_init_all();
    tud_init(0);

    // Initialize Push Buttons
    init_PB();

    // Initialize I2C
    init_I2C();

    // Initialize ADS1115
    // Edit contents of function to fiddle with ADS1115 settings
    init_ads();

    absolute_time_t next = make_timeout_time_ms(500);
    
    while (true) {
        tud_task(); 

        bool ads1_complete = read_ads_channels(&ads1, adc_values_1, &adc1_state);
        bool ads2_complete = read_ads_channels(&ads2, adc_values_2, &adc2_state);
        read_PB();
        
        interpret_midi_state();
        
        // Only send note if current_note has changed
        if (current_note != previous_note) {
            // Send note off for previous note if it was valid
            if (previous_note != -1) {
                send_note_off(previous_note);
            }
            // Send note on for current note if it's valid
            if (current_note != -1) {
                send_note_on(current_note);
            }
            previous_note = current_note;
        }
    }
}

void serial_debug_print() {
    printf("ADS1: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_1[0], adc_values_1[1], adc_values_1[2], adc_values_1[3]);
    printf("ADS2: A0:%5d  A1:%5d  A2:%5d  A3:%5d ", 
            adc_values_2[0], adc_values_2[1], adc_values_2[2], adc_values_2[3]);
    printf("BTN: %d %d %d %d ", buttons[0], buttons[1], buttons[2], buttons[3]);
    printf("MIDI Note: %d (Note %s)\n", current_note, note_on ? "ON" : "OFF");
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
        if (current_time - state->last_switch_time >= 3) {
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
    
    // Read FSR value for volume control based on which button is pressed
    // FSR values are on ADS1 channels 0-3 corresponding to buttons 0-3
    int16_t fsr_value = adc_values_1[pressed_button];
    
    // Convert FSR to MIDI volume and send if changed
    current_volume = fsr_to_volume(fsr_value);
    if (current_volume != previous_volume) {
        send_volume_control(current_volume);
        previous_volume = current_volume;
    }
    
    // Determine which ADC channel corresponds to the softpot for this string
    // Assuming adc_values_2[0] is the softpot for now (you may need to adjust this)
    int16_t softpot_value = adc_values_2[0];
    
    // Get fret position from softpot
    int16_t fret = get_fret_from_softpot(softpot_value);
    
    // Calculate final MIDI note (base note + fret offset)
    current_note = base_note + fret;
    note_on = true;
}

void send_note_on(int16_t note) {
    if (!tud_midi_mounted()) return;

    const uint8_t vel = 100;
    uint8_t msg[3];
    
    msg[0] = 0x90; // Note On
    msg[1] = note;
    msg[2] = vel;
    
    tud_midi_stream_write(0, msg, 3);
}

void send_note_off(int16_t note) {
    if (!tud_midi_mounted()) return;

    uint8_t msg[3];
    
    msg[0] = 0x80; // Note Off
    msg[1] = note;
    msg[2] = 0;
    
    tud_midi_stream_write(0, msg, 3);
}

// Convert FSR value to MIDI volume (0-127)
// FSR is inverse: higher FSR values = lower volume
int16_t fsr_to_volume(int16_t fsr_value) {
    if (fsr_value < FSR_MIN_VALUE) {
        return VOLUME_MAX; // Maximum volume
    } else if (fsr_value > FSR_MAX_VALUE) {
        return VOLUME_MIN; // Minimum volume
    } else {
        // Linear interpolation between FSR_MIN_VALUE and FSR_MAX_VALUE
        // Map FSR_MIN_VALUE->VOLUME_MAX and FSR_MAX_VALUE->VOLUME_MIN
        int16_t fsr_range = FSR_MAX_VALUE - FSR_MIN_VALUE;
        int16_t fsr_offset = fsr_value - FSR_MIN_VALUE;
        int16_t volume = VOLUME_MAX - ((fsr_offset * (VOLUME_MAX - VOLUME_MIN)) / fsr_range);
        return volume;
    }
}

// Send MIDI volume control change (CC7)
void send_volume_control(int16_t volume) {
    if (!tud_midi_mounted()) return;

    uint8_t msg[3];
    
    msg[0] = 0xB0; // Control Change on channel 1
    msg[1] = 0x07; // CC7 (Main Volume)
    msg[2] = volume & 0x7F; // Ensure 7-bit value
    
    tud_midi_stream_write(0, msg, 3);
}