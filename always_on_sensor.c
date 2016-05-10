#include <stdlib.h>

#include "always_on_sensor.h"

void *ALWAYS_ON_SNR_open(struct config_t *cfg)
{
	return NULL;
}

int ALWAYS_ON_SNR_IsActive(void *snr)
{
	return 1;
}

void ALWAYS_ON_SNR_destroy(void *snr)
{
	return;
}

