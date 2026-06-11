CC = gcc
CFLAGS = -Wall -Wextra -O3
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = pathfinder

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
