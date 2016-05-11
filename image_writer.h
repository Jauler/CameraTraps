#ifndef IMAGE_WRITER_H_INCLUDED
#define IMAGE_WRITER_H_INCLUDED

#include "config.h"

enum format_e {
	IMG_WR_FMT_GRAYSCALE,
	IMG_WR_FMT_RGB,
	IMG_WR_FMT_YUV
};

struct image_t {
	int width;
	int height;
	int num_pixel_components;
	enum format_e format;
	const void *data;
};

int write_image(const char *filename, const struct image_t *img, struct config_t *cfg);

#endif
