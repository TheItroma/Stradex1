#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ads1115.h"

#include "bsp/board.h"
#include "tusb.h"

////////////////////// HARDWARE DEFINITIONS //////////////////////
// I2C Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

// ADS1115 Addresses
#define ADS_1_ADDR 0x48
#define ADS_2_ADDR 0x49

// ADS Objects
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

// USB device status
static bool device_mounted = false;

// ADS1115 Channel Configurations
uint16_t mux_configs[4] = {
    ADS1115_MUX_SINGLE_0,
    ADS1115_MUX_SINGLE_1,
    ADS1115_MUX_SINGLE_2,
    ADS1115_MUX_SINGLE_3
};

////////////////////// MIDI DEFINITIONS //////////////////////
// Open string notes (G3, D4, A4, E5)
const int open_notes[] = {55, 62, 69, 76};

// Fingerboard configuration
#define NUM_FRETS 15  // Number of frets/semitones available on fingerboard

// Fret boundary positions on soft pot (0-26000 range)
// These values define where each fret begins - tune these to match your soft pot
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

// Define functions
void serial_debug_print();
void init_I2C();
void init_PB();
void init_ads();
void read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values);
void read_PB();
void midi_note_handler();

int main()
{
    // Initialize tinyusb
    board_init();
    tusb_init();
    
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
        // Run tinyusb task
        tud_task();

        // Read all sensors
        read_PB();
        read_ads_channels(&ads1, adc_values_1);
        read_ads_channels(&ads2, adc_values_2);

        // Run MIDI note handler
        // It constantly checks for the sensor data and adjusts midi note on and offs accordingly
        midi_note_handler();
    }
}

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

////////////////////// MIDI FUNCTIONS //////////////////////
// Helper function to get fret number from soft pot position
int get_fret_from_position(int soft_pot_value) {
    // Handle negative values more aggressively - they indicate no touch or error
    if (soft_pot_value < 0) {
        return 0; // Default to open string for negative values
    }
    
    // Clamp to maximum range
    if (soft_pot_value > 26000) {
        soft_pot_value = 26000;
    }
    
    // Find which fret this position corresponds to
    for (int i = 0; i < NUM_FRETS; i++) {
        if (soft_pot_value >= fret_boundaries[i] && soft_pot_value < fret_boundaries[i + 1]) {
            return i;
        }
    }
    
    // If we're at or above the last boundary, return the highest fret
    return NUM_FRETS - 1;
}

// Helper function to calculate the MIDI note for a string + fret combination
int get_violin_note(int string_index, int fret) {
    return open_notes[string_index] + fret;
}

// Helper function to safely turn a note ON with violin fingerboard logic
void safe_violin_note_on(int button_index, int fret, bool note_on_state[], int current_notes[]) {
  if (!note_on_state[button_index]) {
    int note_to_play = get_violin_note(button_index, fret);
    uint8_t msg[3];
    msg[0] = 0x90;                              // Note On - Channel 1
    msg[1] = note_to_play;                      // Calculate note based on string + fret
    msg[2] = 127;                               // Velocity
    tud_midi_n_stream_write(0, 0, msg, 3);
    note_on_state[button_index] = true;
    current_notes[button_index] = note_to_play; // Store the actual MIDI note being played
  }
}

// Helper function to safely turn a note OFF with violin fingerboard logic
void safe_violin_note_off(int button_index, bool note_on_state[], int current_notes[]) {
  if (note_on_state[button_index]) {
    uint8_t msg[3];
    msg[0] = 0x80;                              // Note Off - Channel 1
    msg[1] = current_notes[button_index];       // Use the stored MIDI note
    msg[2] = 0;                                 // Velocity
    tud_midi_n_stream_write(0, 0, msg, 3);
    note_on_state[button_index] = false;
    current_notes[button_index] = -1;           // Clear the stored note
  }
}

// Helper function to remove button index from stack
void remove_from_stack(int button_index, int note_stack[], int *stack_size) {
  for (int j = 0; j < *stack_size; j++) {
    if (note_stack[j] == button_index) {
      // Shift remaining button indices down
      for (int k = j; k < *stack_size - 1; k++) {
        note_stack[k] = note_stack[k + 1];
      }
      (*stack_size)--;
      break;
    }
  }
}

void midi_note_handler(void)
{
  static bool prev_button_states[NUM_PUSHBUTTONS] = {false};
  static int note_stack[NUM_PUSHBUTTONS]; // Stack to remember note order (stores button indices)
  static int stack_size = 0; // Current number of notes in stack
  static bool note_on_state[NUM_PUSHBUTTONS] = {false}; // Track which buttons have notes ON
  static int current_notes[NUM_PUSHBUTTONS] = {-1, -1, -1, -1}; // Track actual MIDI notes being played
  static int prev_fret = 0; // Track previous fret position for fret changes
  static int stable_fret_count = 0; // Counter for fret stability
  static int candidate_fret = 0; // Candidate fret for stability check
  
  // Get current fret position from soft pot (ADS2 CH4)
  int raw_fret = get_fret_from_position(adc_values_2[3]); // CH4 is index 3
  
  // Add stability filtering - only change fret if it's stable for a few readings
  int current_fret;
  if (raw_fret == candidate_fret) {
    stable_fret_count++;
    if (stable_fret_count >= 3) { // Require 3 consistent readings
      current_fret = raw_fret;
    } else {
      current_fret = prev_fret; // Keep previous fret until stable
    }
  } else {
    candidate_fret = raw_fret;
    stable_fret_count = 1;
    current_fret = prev_fret; // Keep previous fret until stable
  }

  // Handle fret changes ONLY if fret actually changed and we have a note playing
  if (current_fret != prev_fret && stack_size > 0) {
    // Find which button is currently playing (top of stack)
    int active_button = note_stack[stack_size - 1];
    if (note_on_state[active_button]) {
      // Turn off old note and turn on new note at new fret position
      safe_violin_note_off(active_button, note_on_state, current_notes);
      safe_violin_note_on(active_button, current_fret, note_on_state, current_notes);
    }
  }
  
  // Check for new button presses
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    // Check if button was just pressed (transition from false to true)
    if (buttons[i] && !prev_button_states[i]) {
      // Turn off all currently playing notes (monophonic behavior)
      for (int j = 0; j < NUM_PUSHBUTTONS; j++) {
        if (note_on_state[j]) {
          safe_violin_note_off(j, note_on_state, current_notes);
        }
      }
      
      // Add new button to stack (store button index, not note value)
      note_stack[stack_size] = i;
      stack_size++;
      
      // Turn on new note with current fret position
      safe_violin_note_on(i, current_fret, note_on_state, current_notes);
    }
  }

  // Check for button releases
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i] && prev_button_states[i]) {
      // Turn off the released note and remove from stack
      safe_violin_note_off(i, note_on_state, current_notes);
      remove_from_stack(i, note_stack, &stack_size); // Remove button index from stack
      
      // If there are still buttons in the stack, play the most recent one
      if (stack_size > 0) {
        int next_button = note_stack[stack_size - 1];
        if (buttons[next_button]) { // Only if the button is still pressed
          safe_violin_note_on(next_button, current_fret, note_on_state, current_notes);
        }
      }
    }
  }
  
  // Safety check: Ensure unpressed buttons have notes off and clean up stack
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i] && note_on_state[i]) {
      safe_violin_note_off(i, note_on_state, current_notes);
      remove_from_stack(i, note_stack, &stack_size);
    }
  }
  
  // Ensure only one note is on (the top of stack, if any)
  if (stack_size > 0) {
    int should_be_on_button = note_stack[stack_size - 1];
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (i == should_be_on_button && buttons[i] && !note_on_state[i]) {
        safe_violin_note_on(i, current_fret, note_on_state, current_notes);
      } else if (i != should_be_on_button && note_on_state[i]) {
        safe_violin_note_off(i, note_on_state, current_notes);
      }
    }
  } else {
    // Turn off all notes if stack is empty
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (note_on_state[i]) {
        safe_violin_note_off(i, note_on_state, current_notes);
      }
    }
  }
  
  // Update previous states
  prev_fret = current_fret;
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    prev_button_states[i] = buttons[i];
  }
}
