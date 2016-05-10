#ifndef IMAGE_WRITER_H_INCLUDED
#define IMAGE_WRITER_H_INCLUDED

#include "config.h"

struct image_t {
	int width;
	int height;
	int format;
	int bits_per_pixel;
	const void *data;
};

int write_image(const char *filename, const struct image_t *img, struct config_t *cfg);

#endif
