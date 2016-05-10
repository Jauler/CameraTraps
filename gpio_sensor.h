#ifndef GPIO_SENSOR_H_INCLUDED
#define GPIO_SENSOR_H_INCLUDED

#include "config.h"

struct gpio_sensor_t;

struct gpio_sensor_t *GPIO_SNR_open(struct config_t *cfg);

int GPIO_SNR_IsActive(struct gpio_sensor_t *snr);

void GPIO_SNR_destroy(struct gpio_sensor_t *snr);

#endif
