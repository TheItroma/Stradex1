#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "config.h"
#include "lib/ads1115/ads1115.h"

// Hardware initialization functions
void init_I2C(void);
void init_PB(void);
void init_ads(void);

// Sensor reading functions
void read_PB(void);
void read_ads_channels(ads1115_adc_t *ads, int16_t *adc_values);

#endif // _HARDWARE_H_
