#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <jpeglib.h>

#include "image_writer.h"
#include "errors.h"
#include "config.h"

#define DEFAULT_QUALITY 85

int writer_to_libjpeg_format(enum format_e fmt)
{
	switch(fmt){
	case IMG_WR_FMT_RGB:
		return JCS_RGB;
	case IMG_WR_FMT_YUV:
		return JCS_YCbCr;
	case IMG_WR_FMT_GRAYSCALE:
		return JCS_GRAYSCALE;

	default:
		return JCS_UNKNOWN;
	}
}

int write_image(const char *filename, const struct image_t *img, struct config_t *cfg)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int quality;

	char *str_quality = NULL;
	if ((str_quality = CFG_GetValue(cfg, "jpeg_quality")) == NULL)
		quality = DEFAULT_QUALITY;
	else
		quality = atoi(str_quality);

	if (quality <= 0 || quality > 100)
		quality = DEFAULT_QUALITY;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	FILE *out = fopen(filename, "wb");
	if (out == NULL){
		WARN(errno, "Could not open output file");
		return -errno;
	}
	jpeg_stdio_dest(&cinfo, out);

	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = img->num_pixel_components;
	cinfo.in_color_space = writer_to_libjpeg_format(img->format);

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW scanline[1] = {(JSAMPROW)img->data};
	while (cinfo.next_scanline < cinfo.image_height){
		jpeg_write_scanlines(&cinfo, scanline, 1);
		scanline[0] = (JSAMPROW)img->data + cinfo.next_scanline * img->width * img->num_pixel_components;
	}

	jpeg_finish_compress(&cinfo);
	fclose(out);

	jpeg_destroy_compress(&cinfo);
	
	return 0;
}


