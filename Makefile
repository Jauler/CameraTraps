
NAME = cameraTraps

CC = gcc
RM = rm

CFLAGS += -Wall
CFLAGS += -Werror

LDFLAGS += $(CFLAGS)
LDFLAGS += -lv4l2 -lv4lconvert -ljpeg

SRC += config.c
SRC += v4l2_camera.c
SRC += image_writer.c
SRC += cameraTraps.c
SRC += sensor.c
SRC += gpio_sensor.c
SRC += always_on_sensor.c

OBJS = $(patsubst %.c, %.o, $(SRC))

.PHONY: clean

all: binary

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

binary: $(OBJS)
	$(CC) $(OBJS) -o $(NAME) $(LDFLAGS)

clean:
	$(RM) -f $(NAME)
	$(RM) -f $(OBJS)


