#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libv4l2.h>
#include <libv4lconvert.h>

#include "errors.h"
#include "v4l2_camera.h"

#define BUFFER_COUNT		2

struct framesize_t
{
	unsigned int width;
	unsigned int height;
};

struct camera_t
{
	int fd;
	struct v4l2_capability caps;
	struct v4l2_buffer buff[BUFFER_COUNT];
	void *mmap[BUFFER_COUNT];
	int current_buff_idx;
};

static int safe_ioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = v4l2_ioctl(fd, request, arg);
	} while (ret == -1 && errno == EINTR);

	return ret;
}

static struct framesize_t CAM_GetHighestSupportedResolution(struct camera_t *cam)
{
	struct framesize_t max_framesize = {0,0};

	//Figure out pixelformat
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmtdesc.index = 0;
	if (safe_ioctl(cam->fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0){
		WARN(errno, "Error: no native format found");
		return max_framesize;
	}

	struct v4l2_frmsizeenum framesize;
	framesize.index = 0;
	framesize.pixel_format = fmtdesc.pixelformat;
	if (safe_ioctl(cam->fd, VIDIOC_ENUM_FRAMESIZES, &framesize) != 0){
		WARN(errno, "Error: Could not enumerate supported resolutions");
		return max_framesize;
	}

	switch(framesize.type){

	case V4L2_FRMSIZE_TYPE_DISCRETE:
		do {
			if (framesize.discrete.width * framesize.discrete.height >
						max_framesize.width * max_framesize.height)
			{
				max_framesize.width = framesize.discrete.width;
				max_framesize.height = framesize.discrete.height;
			}

			framesize.index++;
		} while (safe_ioctl(cam->fd, VIDIOC_ENUM_FRAMESIZES, &framesize) == 0);
		break;

	case V4L2_FRMSIZE_TYPE_CONTINUOUS:
	case V4L2_FRMSIZE_TYPE_STEPWISE:
		max_framesize.width  = framesize.stepwise.max_width;
		max_framesize.height = framesize.stepwise.max_height;
		break;
	}

	return max_framesize;
}


struct camera_t *CAM_open(struct config_t *cfg)
{
	int i;
	struct camera_t *cam = NULL;

	char *filename = CFG_GetValue(cfg, "video_device");
	if (!filename){
		ERR_NOCFG("video_device");
		goto ERR;
	}

	cam = calloc(1, sizeof(struct camera_t));
	if (!cam){
		WARN(ENOMEM, "Error while creating camera");
		goto ERR;
	}

	cam->fd = v4l2_open(filename, O_RDWR);
	if (cam->fd < 0)
	{
		WARN(errno, "Error while opening camera device");
		goto ERR;
	}

	if (safe_ioctl(cam->fd, VIDIOC_QUERYCAP, &cam->caps) == -1){
		WARN(EINVAL, "Error while reading camera capabilities");
		goto ERR;
	}

	if (!(cam->caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		WARN(EINVAL, "Error: device is incapabale of single-planar capture");
		goto ERR;
	}

	if (!(cam->caps.capabilities & V4L2_CAP_STREAMING)){
		WARN(EINVAL, "Error: device is incapable of streaming");
		goto ERR;
	}

	struct framesize_t framesize = CAM_GetHighestSupportedResolution(cam);
	if (framesize.height * framesize.width <= 0){
		WARN(EINVAL, "Error: Pixel resolution search failed");
		goto ERR;
	}

	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = framesize.width;
	format.fmt.pix.height = framesize.height;
	if (safe_ioctl(cam->fd, VIDIOC_S_FMT, &format) == -1){
		WARN(EINVAL, "Error: Format set failed");
		return NULL;
	}

	struct v4l2_requestbuffers bufreq;
	bufreq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufreq.memory = V4L2_MEMORY_MMAP;
	bufreq.count = BUFFER_COUNT;
	if (safe_ioctl(cam->fd, VIDIOC_REQBUFS, &bufreq) == -1){
		WARN(EINVAL, "Error: Buffer request failed");
		return NULL;
	}

	for (i = 0; i < BUFFER_COUNT; i++){
		memset(&cam->buff[i], 0, sizeof(cam->buff[i]));
		cam->buff[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cam->buff[i].memory = V4L2_MEMORY_MMAP;
		cam->buff[i].index = i;
		if (safe_ioctl(cam->fd, VIDIOC_QUERYBUF, &cam->buff[i]) == -1){
			WARN(EINVAL, "Error: Buffer query failed");
			return NULL;
		}

		cam->mmap[i] = v4l2_mmap(NULL, cam->buff[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, cam->buff[i].m.offset);
		if (cam->mmap[i] == MAP_FAILED){
			WARN(EINVAL, "Error: memory mapping failed");
			return NULL;
		}
	}

	return cam;

ERR:
	free(cam);
	return NULL;
}

void CAM_prepare(struct camera_t *cam)
{
	//spin single frame - some cameras require queued buffers before STREAMON
	memset(cam->mmap[cam->current_buff_idx], 0, cam->buff[cam->current_buff_idx].length);
	if (safe_ioctl(cam->fd, VIDIOC_QBUF, &cam->buff[cam->current_buff_idx]) == -1){
		WARN(EINVAL, "Error: Buffer queue failed");
		return;
	}

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (safe_ioctl(cam->fd, VIDIOC_STREAMON, &type) == -1){
		WARN(EINVAL, "Error: Stream start failed");
		return;
	}

	if (safe_ioctl(cam->fd, VIDIOC_DQBUF, &cam->buff[cam->current_buff_idx]) == -1){
		WARN(EINVAL, "Error: Buffer dequeue failed");
		return;
	}

	cam->current_buff_idx = (cam->current_buff_idx + 1) % BUFFER_COUNT;

	return;
}

struct camera_buffer_t CAM_capture(struct camera_t *cam)
{
	struct camera_buffer_t buff = {NULL, 0};

	cam->current_buff_idx = (cam->current_buff_idx + 1) % BUFFER_COUNT;
	memset(cam->mmap[cam->current_buff_idx], 0, cam->buff[cam->current_buff_idx].length);
	if (safe_ioctl(cam->fd, VIDIOC_QBUF, &cam->buff[cam->current_buff_idx]) == -1){
		WARN(EINVAL, "Error: Buffer dequeue failed");
		return buff;
	}

	if (safe_ioctl(cam->fd, VIDIOC_DQBUF, &cam->buff[cam->current_buff_idx]) == -1){
		WARN(EINVAL, "Error: Buffer queue failed");
		return buff;
	}

	buff.buffer = cam->mmap[cam->current_buff_idx];
	buff.length = cam->buff[cam->current_buff_idx].length;
	return buff;
}

void CAM_unprepare(struct camera_t *cam)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (safe_ioctl(cam->fd, VIDIOC_STREAMOFF, &type) == -1){
		WARN(EINVAL, "Error: Stream stop failed");
		return;
	}
}

void CAM_destroy(struct camera_t *cam)
{
	int i;
	for (i = 0; i < BUFFER_COUNT; i++)
		if (cam->mmap[i])
			munmap(cam->mmap[i], cam->buff[i].length);
	close(cam->fd);
	free(cam);
	return;
}

