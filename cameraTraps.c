#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include "config.h"
#include "v4l2_camera.h"
#include "sensor.h"
#include "errors.h"

struct config_t *cfg = NULL;
struct camera_t *cam = NULL;
struct sensor_t *snr = NULL;


static void signal_cleanup(int signum)
{
	if (signum != 0){
		exit(EXIT_FAILURE);
		return;
	}
}

static void atexit_cleanup(void)
{
	if (cam){
		CAM_unprepare(cam);
		CAM_destroy(cam);
	}

	SNR_destroy(snr);
	CFG_destroy(cfg);
	return;
}

static void write_file(char *filename, struct camera_buffer_t buff)
{
	FILE *f = fopen(filename, "w");
	if (f == NULL)
		ERR(-1, errno, "Error: Opening image file");

	fprintf(f, "P6\n%d %d 255\n", 640, 480);
	fwrite(buff.buffer, buff.length, 1, f);
	fclose(f);
}

int main(int argc, char *argv[])
{
	char *config_file = NULL;

	atexit(atexit_cleanup);
	signal(SIGINT, signal_cleanup);

	if (argc < 2)
		config_file = "/etc/cameraTraps.conf";
	else
		config_file = argv[1];

	cfg = CFG_ParseConfigFile(config_file);
	if (!cfg)
		exit(-1);
	char *photo_dir = CFG_GetValue(cfg, "photo_dir");
	if (!photo_dir){
		ERR_NOCFG("photo_dir");
		exit(-1);
	}

	cam = CAM_open(cfg);
	if (!cam)
		exit(-1);
	snr = SNR_open(cfg);
	if (!snr)
		exit(-1);


	char filename[256];
	char date[64];
	struct camera_buffer_t buff;
	struct timespec ts = {0, 100000000};
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

	return 0;
}
