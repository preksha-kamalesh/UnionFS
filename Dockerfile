# ============================================================
# Dockerfile — Mini-UnionFS build & test environment
# Base: Ubuntu 22.04  |  FUSE 2.9.x
#
# Works on: macOS, Windows (Docker Desktop), Linux
#
# Build image:
#   docker compose build
#
# Drop into a shell:
#   docker compose run --rm dev
#
# Build the project inside the container:
#   make
# ============================================================

FROM ubuntu:22.04

# Avoid interactive prompts during apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools + FUSE dev library
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    libfuse-dev \
    fuse \
    && rm -rf /var/lib/apt/lists/*

# Allow FUSE to run as non-root inside the container
RUN echo "user_allow_other" >> /etc/fuse.conf

WORKDIR /app

# Copy source into the image (used for CI; live edits use volume mount instead)
COPY . .

# Default command: drop into bash so you can run make, test, etc.
CMD ["/bin/bash"]
