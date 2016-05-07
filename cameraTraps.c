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

#define DEFAULT_PHOTO_DELAY			200
#define DEFAULT_CONFIG_FILE			"/etc/cameraTraps.conf"

struct config_t *cfg = NULL;
struct camera_t *cam = NULL;
struct sensor_t *snr = NULL;
int shouldExit = 0;

static void signal_cleanup(int signum)
{
	shouldExit = 1;
	return;
}

static void atexit_cleanup(void)
{
	if (cam){
		CAM_unprepare(cam);
		CAM_destroy(cam);
	}

	if (snr)
		SNR_destroy(snr);

	if (cfg)
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
	unsigned long long int delay;

	atexit(atexit_cleanup);
	signal(SIGINT, signal_cleanup);

	if (argc < 2)
		config_file = DEFAULT_CONFIG_FILE;
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

	char *str_delay = NULL;
	if ((str_delay = CFG_GetValue(cfg, "photo_delay")) != NULL){
		unsigned long long int tmp_delay = atoi(str_delay);
		if (tmp_delay < 100 || tmp_delay > 1000){
			WARN(EINVAL, "Photo delay is either invalid or not in range [100, 1000]");
			exit(EXIT_FAILURE);
		}
	} else {
		delay = DEFAULT_PHOTO_DELAY;
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
	struct timespec long_ts = {delay / 1000, delay * 1000000 % 1000000000};
	int isActive = 0, old_isActive = 0;
	int counter = 0;

	CAM_prepare(cam);
	while (1){
		nanosleep(&ts, NULL);

		if (shouldExit)
			break;

		isActive = SNR_IsActive(snr);
		if (isActive && !old_isActive)
			counter = 0;
		old_isActive = isActive;

		if (!isActive)
			continue;

		counter++;

		buff = CAM_capture(cam);
		if (buff.length <= 0)
			continue;

		time_t t = time(NULL);
		strftime(date, sizeof(date), "%Y%m%d_%H%M", localtime(&t));
		snprintf(filename, sizeof(filename), "%s/%s_%d.ppm", photo_dir, date, counter);
		printf("Making photo %s\n", filename);
		write_file(filename, buff);

		nanosleep(&long_ts, NULL);
	}

	return 0;
}
