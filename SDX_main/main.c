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
const int open_notes[] = {55, 62, 69, 76};

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











// Helper function to safely turn a note ON
void safe_note_on(int button_index, bool note_on_state[]) {
  if (!note_on_state[button_index]) {
    uint8_t msg[3];
    msg[0] = 0x90;                    // Note On - Channel 1
    msg[1] = open_notes[button_index]; // Note Number
    msg[2] = 127;                     // Velocity
    tud_midi_n_stream_write(0, 0, msg, 3);
    note_on_state[button_index] = true;
  }
}

// Helper function to safely turn a note OFF
void safe_note_off(int button_index, bool note_on_state[]) {
  if (note_on_state[button_index]) {
    uint8_t msg[3];
    msg[0] = 0x80;                    // Note Off - Channel 1
    msg[1] = open_notes[button_index]; // Note Number
    msg[2] = 0;                       // Velocity
    tud_midi_n_stream_write(0, 0, msg, 3);
    note_on_state[button_index] = false;
  }
}

void midi_note_handler(void)
{
  static bool prev_button_states[NUM_PUSHBUTTONS] = {false};
  static int note_stack[NUM_PUSHBUTTONS]; // Stack to remember note order
  static int stack_size = 0; // Current number of notes in stack
  static bool note_on_state[NUM_PUSHBUTTONS] = {false}; // Track which notes are actually ON

  // Check for new button presses first
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    // Check if button was just pressed (transition from false to true)
    if (buttons[i] && !prev_button_states[i]) {
      // Turn off current note if one is playing
      if (stack_size > 0) {
        // Find which button corresponds to the current top note
        int current_note = note_stack[stack_size - 1];
        for (int j = 0; j < NUM_PUSHBUTTONS; j++) {
          if (open_notes[j] == current_note) {
            safe_note_off(j, note_on_state);
            break;
          }
        }
      }
      
      // Add new note to stack
      note_stack[stack_size] = open_notes[i];
      stack_size++;
      
      // Turn on new note
      safe_note_on(i, note_on_state);
    }
  }

  // Check for button releases
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i] && prev_button_states[i]) {
      // Find and remove this note from the stack
      int note_to_remove = open_notes[i];
      bool found = false;
      
      for (int j = 0; j < stack_size; j++) {
        if (note_stack[j] == note_to_remove) {
          found = true;
          // Shift remaining notes down
          for (int k = j; k < stack_size - 1; k++) {
            note_stack[k] = note_stack[k + 1];
          }
          stack_size--;
          break;
        }
      }
      
      if (found) {
        // Turn off the released note
        safe_note_off(i, note_on_state);
        
        // If there are still notes in the stack, play the most recent one
        if (stack_size > 0) {
          int next_note = note_stack[stack_size - 1];
          for (int j = 0; j < NUM_PUSHBUTTONS; j++) {
            if (open_notes[j] == next_note) {
              safe_note_on(j, note_on_state);
              break;
            }
          }
        }
      }
    }
  }
  
  // Comprehensive safety check: Ensure consistency between button states and note states
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i]) {
      // If button is not pressed, its note MUST be off
      if (note_on_state[i]) {
        safe_note_off(i, note_on_state);
        
        // Also remove from stack if present
        for (int j = 0; j < stack_size; j++) {
          if (note_stack[j] == open_notes[i]) {
            // Shift remaining notes down
            for (int k = j; k < stack_size - 1; k++) {
              note_stack[k] = note_stack[k + 1];
            }
            stack_size--;
            break;
          }
        }
      }
    }
  }
  
  // Additional safety: Ensure only the top stack note should be on
  if (stack_size > 0) {
    int should_be_on_note = note_stack[stack_size - 1];
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (open_notes[i] == should_be_on_note) {
        // This note should be on
        if (!note_on_state[i] && buttons[i]) {
          safe_note_on(i, note_on_state);
        }
      } else {
        // All other notes should be off
        if (note_on_state[i]) {
          safe_note_off(i, note_on_state);
        }
      }
    }
  } else {
    // No notes should be on if stack is empty
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (note_on_state[i]) {
        safe_note_off(i, note_on_state);
      }
    }
  }
  
  // Update previous button states for all buttons
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    prev_button_states[i] = buttons[i];
  }
}













