#ifndef V4L2_CAMERA_H_INCLUDED
#define V4L2_CAMERA_H_INCLUDED

struct camera_t;

struct camera_t *CAM_open(char *filename);

void CAM_prepare(struct camera_t *cam);
void *CAM_capture(struct camera_t *cam);
void CAM_unprepare(struct camera_t *cam);

int CAM_GetBufferSize(struct camera_t *cam, int index);

void CAM_destroy(struct camera_t *cam);

#endif
