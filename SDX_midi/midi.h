#ifndef _MIDI_H_
#define _MIDI_H_

#include <stdbool.h>
#include <stdint.h>

// External variable declarations (defined in main.c)
extern bool buttons[];
extern int16_t adc_values_1[];
extern int16_t adc_values_2[];
extern const int open_notes[];
extern const int fret_boundaries[];

// Constants
#define NUM_PUSHBUTTONS 4
#define NUM_FRETS 16

// MIDI helper functions
int get_fret_from_position(int soft_pot_value);
int get_violin_note(int string_index, int fret);

// MIDI note control functions
void safe_violin_note_on(int button_index, int fret, bool note_on_state[], int current_notes[]);
void safe_violin_note_off(int button_index, bool note_on_state[], int current_notes[]);
void remove_from_stack(int button_index, int note_stack[], int *stack_size);

// Main MIDI handler
void midi_note_handler(void);

#endif // _MIDI_H_
