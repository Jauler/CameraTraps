#ifndef SENSOR_H_INCLUDED
#define SENSOR_H_INCLUDED

#include "config.h"

struct sensor_t;

struct sensor_t *SNR_open(struct config_t *cfg);

int SNR_IsActive(struct sensor_t *snr);

void SNR_destroy(struct sensor_t *snr);

#endif
