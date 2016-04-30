#ifndef V4L2_CAMERA_H_INCLUDED
#define V4L2_CAMERA_H_INCLUDED

#include "config.h"

struct camera_t;

struct camera_buffer_t
{
	const void *buffer;
	unsigned long int length;
};

struct camera_t *CAM_open(struct config_t * cfg);

void CAM_prepare(struct camera_t *cam);
struct camera_buffer_t CAM_capture(struct camera_t *cam);
void CAM_unprepare(struct camera_t *cam);

void CAM_destroy(struct camera_t *cam);

#endif
