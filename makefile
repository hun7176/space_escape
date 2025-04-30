CC = gcc
CFLAGS = -Wall -Wextra -lncursesw
TARGET = main
SRCS = main.c func.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
