#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "lib/ads1115/ads1115.h"

////////////////////// HARDWARE DEFINITIONS //////////////////////
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
extern const int PB[];
#define NUM_PUSHBUTTONS 4
#define PB_1 PB[0]
#define PB_2 PB[1] 
#define PB_3 PB[2]
#define PB_4 PB[3]

////////////////////// MIDI DEFINITIONS //////////////////////
// Fingerboard configuration
#define NUM_FRETS 16  // Number of frets/semitones available on fingerboard

// Global variables declarations
extern ads1115_adc_t ads1, ads2;
extern bool buttons[NUM_PUSHBUTTONS];
extern int16_t adc_values_1[4];
extern int16_t adc_values_2[4];
extern bool device_mounted;
extern uint16_t mux_configs[4];
extern const int open_notes[];
extern const int fret_boundaries[NUM_FRETS + 1];

#endif // _CONFIG_H_
