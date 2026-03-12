CC = gcc
CFLAGS = -Wall -Wextra -g -D_FILE_OFFSET_BITS=64
FUSE_CFLAGS = $(shell pkg-config --cflags fuse)
FUSE_LIBS = $(shell pkg-config --libs fuse)

LDFLAGS = $(FUSE_LIBS)

TARGET = mini_unionfs
SOURCES = src/mini_unionfs.c src/path_resolution.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

install: $(TARGET)
	@echo "To mount the filesystem, run:"
	@echo "./$(TARGET) <lower_dir> <upper_dir> <mount_point>"

test: $(TARGET)
	@bash tests/test_unionfs.sh

.PHONY: all clean install test

