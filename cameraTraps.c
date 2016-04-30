#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "config.h"
#include "v4l2_camera.h"
#include "sensor.h"
#include "errors.h"

static void write_file(char *filename, struct camera_buffer_t buff)
{
	int ret;
	unsigned int count = 0;
	int jpgfile = open(filename, O_WRONLY | O_CREAT, 0660);
	if (jpgfile < 0)
		ERR(-1, errno, "Error: Opening image file");

	while (buff.length > count){
		ret = write(jpgfile, buff.buffer + count, buff.length - count);
		if (ret <= 0){
			WARN(errno, "Error: File write failed");
			return;
		}

		count += ret;
	}
	close(jpgfile);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <configuration file>\n", argv[0]);
		exit(0);
	}

	struct config_t *cfg = CFG_ParseConfigFile(argv[1]);
	if (!cfg)
		exit(-1);
	char *photo_dir = CFG_GetValue(cfg, "photo_dir");
	if (!photo_dir){
		ERR_NOCFG("photo_dir");
		exit(-1);
	}

	struct camera_t *cam = CAM_open(cfg);
	if (!cam)
		exit(-1);
	struct sensor_t *snr = SNR_open(cfg);
	if (!snr)
		exit(-1);


	char filename[256];
	char date[64];
	struct camera_buffer_t buff;
	struct timespec ts = {0, 10000};
	int isActive = 0, old_isActive = 0;
	int counter = 0;

	CAM_prepare(cam);
	while (1){
		nanosleep(&ts, NULL);

		isActive = SNR_IsActive(snr);
		if (isActive && !old_isActive){
			// rising edge
			counter = 0;
			time_t t = time(NULL);
			strftime(date, sizeof(date), "%Y%m%d_%H%M", localtime(&t));
		}
		old_isActive = isActive;

		if (!isActive)
			continue;

		counter++;

		printf("Making photo\n");

		buff = CAM_capture(cam);

		snprintf(filename, sizeof(filename), "%s/%s_%d.jpeg", photo_dir, date, counter);
		write_file(filename, buff);
	}

	CAM_unprepare(cam);

	SNR_destroy(snr);
	CAM_destroy(cam);
	CFG_destroy(cfg);
	return 0;
}
