#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "errors.h"
#include "sensor.h"

struct sensor_t
{
	int fd;
	int reverse;
	char *str_gpio_num;
	int gpio_num;
	char *value_file;
};

struct sensor_t *GPIO_SNR_open(struct config_t *cfg)
{
	struct sensor_t *snr = calloc(1, sizeof(struct sensor_t));

	char *str_active = CFG_GetValue(cfg, "active");
	if (!str_active) {
		ERR_NOCFG("active");
		goto ERR;
	}
	if (strcmp(str_active, "high") == 0) {
		snr->reverse = 0;
	} else if (strcmp(str_active, "low") == 0) {
		snr->reverse = 1;
	} else {
		WARN(EINVAL, "Config Error: \"active\" should be either \"high\" or \"low\"");
		goto ERR;
	}

	snr->value_file = CFG_GetValue(cfg, "value_file");
	if (!snr->value_file) {
		ERR_NOCFG("value_file");
		goto ERR;
	}

	snr->str_gpio_num = CFG_GetValue(cfg, "gpio_num");
	if (!snr->str_gpio_num) {
		ERR_NOCFG("gpio_num");
		goto ERR;
	}
	snr->gpio_num = atoi(snr->str_gpio_num);

	//check if we need to export
	char *export_file = CFG_GetValue(cfg, "export_file");
	if (export_file) {
		int export_fd = open(export_file, O_WRONLY);
		if (export_fd < 0) {
			WARN(errno, "Error exporting");
			goto ERR;
		}

		int ret = write(export_fd, snr->str_gpio_num, strlen(snr->str_gpio_num));
		if (ret <= 0 && errno != EBUSY) {
			WARN(errno, "Error exporting");
			goto ERR;
		}

		close(export_fd);
	}

	char *direction_file = CFG_GetValue(cfg, "direction_file");
	if (direction_file) {
		int direction_fd = open(direction_file, O_WRONLY);
		if (direction_fd < 0) {
			WARN(errno, "Error setting direction");
			goto ERR;
		}

		if (write(direction_fd, "in", 2) <= 0) {
			WARN(errno, "Error setting direction");
			goto ERR;
		}

		close(direction_fd);
	}

	snr->fd = open(snr->value_file, O_RDONLY);
	if (snr->fd < 0) {
		WARN(errno, "Error gpio open failed");
		goto ERR;
	}
	return snr;

ERR:
	free(snr);
	return NULL;
}

int GPIO_SNR_IsActive(struct sensor_t *snr)
{
	char status = 0;
	lseek(snr->fd, 0, SEEK_SET);
	read(snr->fd, &status, 1);
	return  (status == '1') ^ snr->reverse;
}

void GPIO_SNR_destroy(struct sensor_t *snr)
{
	close(snr->fd);
	free(snr);
	return;
}

