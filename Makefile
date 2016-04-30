
NAME = cameraTraps

CC = gcc
RM = rm

CFLAGS += -Wall
CFLAGS += -Werror

LDFLAGS += $(CFLAGS)

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
	$(CC) $(LDFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) -f $(NAME)
	$(RM) -f $(OBJS)


