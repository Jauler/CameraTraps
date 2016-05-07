
NAME = cameraTraps

CC = gcc
RM = rm

CFLAGS += -Wall
CFLAGS += -Werror

LDFLAGS += $(CFLAGS)
LDFLAGS += -lv4l2 -lv4lconvert

SRC += config.c
SRC += v4l2_camera.c
SRC += cameraTraps.c
SRC += sensor.c

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


