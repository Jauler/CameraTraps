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

#include "errors.h"
#include "v4l2_camera.h"

struct camera_t
{
	int fd;
	struct v4l2_capability caps;
	struct v4l2_buffer buff;
	void *mmap;
};

static int safe_ioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while (ret == -1 && errno == EINTR);

	return ret;
}

static int CAM_IsFourCCSupported(struct camera_t * cam, uint32_t fourcc)
{
	struct v4l2_fmtdesc fmtdesc;

	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmtdesc.index = 0;
	while (safe_ioctl(cam->fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0){
		if (fmtdesc.pixelformat == fourcc)
			return 1;

		fmtdesc.index++;
	}

	return 0;
}

struct camera_t *CAM_open(char *filename)
{
	struct camera_t *cam = calloc(1, sizeof(struct camera_t));
	if (!cam)
		ERR(-1, ENOMEM, "Error while creating camera");

	cam->fd = open(filename, O_RDWR);
	if (cam->fd == -1)
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

	if (!CAM_IsFourCCSupported(cam, v4l2_fourcc('J', 'P', 'E', 'G'))){
		WARN(EINVAL, "Error: Camera does not support JPEG format");
		goto ERR;
	}

	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = 640;
	format.fmt.pix.height = 480;
	if (safe_ioctl(cam->fd, VIDIOC_S_FMT, &format) == -1){
		WARN(EINVAL, "Error: Format set failed");
		return NULL;
	}

	struct v4l2_requestbuffers bufreq;
	bufreq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufreq.memory = V4L2_MEMORY_MMAP;
	bufreq.count = 1;
	if (safe_ioctl(cam->fd, VIDIOC_REQBUFS, &bufreq) == -1){
		WARN(EINVAL, "Error: Buffer request failed");
		return NULL;
	}

	memset(&cam->buff, 0, sizeof(cam->buff));
	cam->buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam->buff.memory = V4L2_MEMORY_MMAP;
	cam->buff.index = 0;
	if (safe_ioctl(cam->fd, VIDIOC_QUERYBUF, &cam->buff) == -1){
		WARN(EINVAL, "Error: Buffer query failed");
		return NULL;
	}

	cam->mmap = mmap(NULL, cam->buff.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, cam->buff.m.offset);
	if (cam->mmap == MAP_FAILED){
		WARN(EINVAL, "Error: memory mapping failed");
		return NULL;
	}
	memset(cam->mmap, 0, cam->buff.length);

	return cam;

ERR:
	free(cam);
	return NULL;
}

void CAM_prepare(struct camera_t *cam)
{
	if (safe_ioctl(cam->fd, VIDIOC_QBUF, &cam->buff) == -1){
		WARN(EINVAL, "Error: Buffer queue failed");
		return;
	}

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (safe_ioctl(cam->fd, VIDIOC_STREAMON, &type) == -1){
		WARN(EINVAL, "Error: Stream start failed");
		return;
	}

	return;
}

struct camera_buffer_t CAM_capture(struct camera_t *cam)
{
	struct camera_buffer_t buff = {NULL, 0};

	if (safe_ioctl(cam->fd, VIDIOC_DQBUF, &cam->buff) == -1){
		WARN(EINVAL, "Error: Buffer dequeue failed");
		return buff;
	}

	if (safe_ioctl(cam->fd, VIDIOC_QBUF, &cam->buff) == -1){
		WARN(EINVAL, "Error: Buffer queue failed");
		return buff;
	}

	buff.buffer = cam->mmap;
	buff.length = cam->buff.length;

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
	if (cam->mmap)
		munmap(cam->mmap, cam->buff.length);
	close(cam->fd);
	free(cam);
	return;
}

