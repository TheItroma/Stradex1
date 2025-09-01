#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "bsp/board.h"
#include "tusb.h"

#include "config.h"
#include "hardware.h"
#include "midi.h"

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
