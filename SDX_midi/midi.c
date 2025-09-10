#include "midi.h"
#include "tusb.h"

// Global variable to track current note state
int current_note = -1;  // -1 means no note, otherwise MIDI note number
bool note_should_be_on = false;

////////////////////// LOGIC FUNCTIONS //////////////////////
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

// Logic function to determine what note should be played
void update_note_logic(int button_index, int fret, bool note_on_state[], int current_notes[]) {
  if (!note_on_state[button_index]) {
    int note_to_play = get_violin_note(button_index, fret);
    current_note = note_to_play;
    note_should_be_on = true;
    note_on_state[button_index] = true;
    current_notes[button_index] = note_to_play;
  }
}

// Logic function to determine note should be turned off
void update_note_off_logic(int button_index, bool note_on_state[], int current_notes[]) {
  if (note_on_state[button_index]) {
    current_note = current_notes[button_index];
    note_should_be_on = false;
    note_on_state[button_index] = false;
    current_notes[button_index] = -1;
  }
}

////////////////////// MIDI OUTPUT FUNCTIONS //////////////////////
// Function to send MIDI note on
void send_midi_note_on(int note) {
  uint8_t msg[3];
  msg[0] = 0x90;  // Note On - Channel 1
  msg[1] = note;  // MIDI note number
  msg[2] = 127;   // Velocity
  tud_midi_n_stream_write(0, 0, msg, 3);
}

// Function to send MIDI note off
void send_midi_note_off(int note) {
  uint8_t msg[3];
  msg[0] = 0x80;  // Note Off - Channel 1
  msg[1] = note;  // MIDI note number
  msg[2] = 0;     // Velocity
  tud_midi_n_stream_write(0, 0, msg, 3);
}

// Function to handle MIDI output based on current_note variable
void handle_midi_output() {
  static int last_sent_note = -1;
  static bool last_note_state = false;
  
  // Handle note changes
  if (note_should_be_on && current_note != -1) {
    // Turn off previous note if different
    if (last_note_state && last_sent_note != current_note && last_sent_note != -1) {
      send_midi_note_off(last_sent_note);
    }
    // Turn on new note if not already on
    if (!last_note_state || last_sent_note != current_note) {
      send_midi_note_on(current_note);
      last_sent_note = current_note;
      last_note_state = true;
    }
  } else if (!note_should_be_on && last_note_state) {
    // Turn off current note
    if (last_sent_note != -1) {
      send_midi_note_off(last_sent_note);
      last_note_state = false;
    }
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

void midi_logic_handler(void)
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
  int raw_fret = get_fret_from_position(adc_values_2[0]); // CH4 is index 3
  
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
      // Update logic for fret change
      update_note_off_logic(active_button, note_on_state, current_notes);
      update_note_logic(active_button, current_fret, note_on_state, current_notes);
    }
  }
  
  // Check for new button presses
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    // Check if button was just pressed (transition from false to true)
    if (buttons[i] && !prev_button_states[i]) {
      // Turn off all currently playing notes (monophonic behavior)
      for (int j = 0; j < NUM_PUSHBUTTONS; j++) {
        if (note_on_state[j]) {
          update_note_off_logic(j, note_on_state, current_notes);
        }
      }
      
      // Add new button to stack (store button index, not note value)
      note_stack[stack_size] = i;
      stack_size++;
      
      // Update logic for new note
      update_note_logic(i, current_fret, note_on_state, current_notes);
    }
  }

  // Check for button releases
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i] && prev_button_states[i]) {
      // Update logic for note off and remove from stack
      update_note_off_logic(i, note_on_state, current_notes);
      remove_from_stack(i, note_stack, &stack_size); // Remove button index from stack
      
      // If there are still buttons in the stack, play the most recent one
      if (stack_size > 0) {
        int next_button = note_stack[stack_size - 1];
        if (buttons[next_button]) { // Only if the button is still pressed
          update_note_logic(next_button, current_fret, note_on_state, current_notes);
        }
      }
    }
  }
  
  // Safety check: Ensure unpressed buttons have notes off and clean up stack
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    if (!buttons[i] && note_on_state[i]) {
      update_note_off_logic(i, note_on_state, current_notes);
      remove_from_stack(i, note_stack, &stack_size);
    }
  }
  
  // Ensure only one note is on (the top of stack, if any)
  if (stack_size > 0) {
    int should_be_on_button = note_stack[stack_size - 1];
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (i == should_be_on_button && buttons[i] && !note_on_state[i]) {
        update_note_logic(i, current_fret, note_on_state, current_notes);
      } else if (i != should_be_on_button && note_on_state[i]) {
        update_note_off_logic(i, note_on_state, current_notes);
      }
    }
  } else {
    // Turn off all notes if stack is empty
    for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
      if (note_on_state[i]) {
        update_note_off_logic(i, note_on_state, current_notes);
      }
    }
  }
  
  // Update previous states
  prev_fret = current_fret;
  for (int i = 0; i < NUM_PUSHBUTTONS; i++) {
    prev_button_states[i] = buttons[i];
  }
}
