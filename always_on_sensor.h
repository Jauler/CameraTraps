#ifndef ALWAYS_ON_SENSOR_H_INCLUDED
#define ALWAYS_ON_SENSOR_H_INCLUDED

#include "config.h"


void *ALWAYS_ON_SNR_open(struct config_t *cfg);

int ALWAYS_ON_SNR_IsActive(void *snr);

void ALWAYS_ON_SNR_destroy(void *snr);

#endif
