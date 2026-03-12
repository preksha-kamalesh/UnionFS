# =============================================================================
# Makefile — Mini-UnionFS
# Author  : Preksha (Core FUSE Setup & Mounting)
# Branch  : feature/preksha-fuse-setup
#
# Usage:
#   make              — build the binary
#   make setup-test   — create test layer directories
#   make mount        — mount with test dirs (foreground, Ctrl-C to stop)
#   make unmount      — unmount the test filesystem
#   make clean        — remove build artifacts
#   make install-deps — install libfuse on Ubuntu
# =============================================================================

CC        := gcc
CFLAGS    := -Wall -Wextra -Wpedantic -g \
             $(shell pkg-config fuse --cflags 2>/dev/null || echo "-I/usr/include/fuse")
LDFLAGS   := $(shell pkg-config fuse --libs   2>/dev/null || echo "-lfuse")

TARGET    := mini_unionfs

SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build

SRCS      := $(SRC_DIR)/mini_unionfs.c
OBJS      := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Test directory names (created locally for quick testing)
LOWER_DIR := lower_dir
UPPER_DIR := upper_dir
MNT_DIR   := mnt

# =============================================================================
# Primary targets
# =============================================================================

.PHONY: all
all: $(BUILD_DIR) $(TARGET)
	@echo ""
	@echo "Build successful → ./$(TARGET)"
	@echo "Run 'make setup-test' then 'make mount' to try it out."
	@echo ""

# Link
$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Linked: $@"

# Compile each .c → .o inside build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/mini_unionfs.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@
	@echo "Compiled: $<"

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# =============================================================================
# Testing helpers
# =============================================================================

.PHONY: setup-test
setup-test:
	@mkdir -p $(LOWER_DIR) $(UPPER_DIR) $(MNT_DIR)
	@echo "hello from lower" > $(LOWER_DIR)/hello.txt
	@echo "data in lower"    > $(LOWER_DIR)/data.txt
	@mkdir -p $(LOWER_DIR)/subdir
	@echo "nested file"      > $(LOWER_DIR)/subdir/nested.txt
	@echo ""
	@echo "Test directories ready:"
	@echo "  $(LOWER_DIR)/ — read-only base layer (pre-populated)"
	@echo "  $(UPPER_DIR)/ — read-write layer     (starts empty)"
	@echo "  $(MNT_DIR)/   — mount point"
	@echo ""
	@echo "Run 'make mount' to start Mini-UnionFS (foreground)."
	@echo ""

.PHONY: mount
mount: $(TARGET)
	@echo "Mounting Mini-UnionFS (foreground mode — Ctrl-C to unmount)..."
	./$(TARGET) $(LOWER_DIR) $(UPPER_DIR) $(MNT_DIR) -f

.PHONY: unmount
unmount:
	fusermount -u $(MNT_DIR) && echo "Unmounted $(MNT_DIR)"

# =============================================================================
# Maintenance
# =============================================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned."

.PHONY: clean-test
clean-test:
	@if mountpoint -q $(MNT_DIR) 2>/dev/null; then \
	    fusermount -u $(MNT_DIR); fi
	rm -rf $(LOWER_DIR) $(UPPER_DIR) $(MNT_DIR)
	@echo "Test directories removed."

# Install FUSE development libraries on Ubuntu/Debian
.PHONY: install-deps
install-deps:
	sudo apt-get update
	sudo apt-get install -y libfuse-dev pkg-config build-essential
	@echo "Dependencies installed."

# =============================================================================
# Docker targets (use these on macOS / Windows)
# =============================================================================

.PHONY: docker-build
docker-build:
	docker compose build

# Open an interactive shell in the container (your source is live-mounted)
.PHONY: docker-shell
docker-shell:
	docker compose run --rm dev

# Build the binary inside the container
.PHONY: docker-make
docker-make:
	docker compose run --rm dev make

# Run setup-test then mount inside the container
.PHONY: docker-mount
docker-mount:
	docker compose run --rm dev bash -c 'make setup-test && make mount'

.PHONY: help
help:
	@echo ""
	@echo "Mini-UnionFS — Available make targets"
	@echo "--------------------------------------"
	@echo "  all           Build the mini_unionfs binary (default)"
	@echo "  setup-test    Create lower/upper/mnt test directories"
	@echo "  mount         Mount using test dirs (foreground)"
	@echo "  unmount       Unmount the test filesystem"
	@echo "  clean         Remove build artifacts and binary"
	@echo "  clean-test    Remove test directories (unmounts first)"
	@echo "  install-deps  Install libfuse-dev on Ubuntu/Debian"
	@echo "  --- Docker (macOS/Windows) ---"
	@echo "  docker-build  Build the Docker image"
	@echo "  docker-shell  Open a shell in the container"
	@echo "  docker-make   Compile inside the container"
	@echo "  docker-mount  setup-test + mount inside container"
	@echo "  help          Show this message"
	@echo ""
