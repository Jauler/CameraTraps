#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "sensor.h"
#include "errors.h"

#include "gpio_sensor.h"
#include "always_on_sensor.h"

struct sensor_t
{
	int (*IsActive)(void *obj);
	void (*destroy)(void *obj);
	void *instance;
};

struct sensor_t *SNR_open(struct config_t *cfg)
{
	struct sensor_t *snr = calloc(1, sizeof(*snr));
	if (!snr)
		ERR(-1, ENOMEM, "Could not open sensor");

	char *sensor = CFG_GetValue(cfg, "sensor_type");
	if (!sensor) {
		ERR_NOCFG("sensor_type");
		goto FAILURE;
	}

	if (strcmp(sensor, "gpio") == 0) {
		snr->IsActive = (int (*)(void *))GPIO_SNR_IsActive;
		snr->destroy = (void (*)(void *))GPIO_SNR_destroy;
		snr->instance = GPIO_SNR_open(cfg);
		if (snr->instance == NULL)
			goto FAILURE;
		return snr;
	}

	if (strcmp(sensor, "always_on") == 0) {
		snr->IsActive = ALWAYS_ON_SNR_IsActive;
		snr->destroy = ALWAYS_ON_SNR_destroy;
		return snr;
	}

FAILURE:
	free(snr);
	return NULL;
}

int SNR_IsActive(struct sensor_t *snr)
{
	return snr->IsActive(snr->instance);
}

void SNR_destroy(struct sensor_t *snr)
{
	snr->destroy(snr->instance);
	free(snr);
}
