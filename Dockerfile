# Multi-stage Dockerfile for Mini-UnionFS
# Ensures consistent build environment across MacOS, Windows, and Linux

FROM ubuntu:22.04

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and FUSE development headers
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libfuse-dev \
    fuse \
    pkg-config \
    git \
    vim \
    nano \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /unionfs

# Copy the entire project
COPY . .

# Create non-root user for running FUSE (best practice)
RUN useradd -m -s /bin/bash unionfs && \
    chown -R unionfs:unionfs /unionfs

# Expose FUSE capabilities for the container
# This allows FUSE operations to work properly
RUN echo "user_allow_other" >> /etc/fuse.conf

# Build the project by default
RUN make clean && make

# Document available commands
RUN echo "=== Mini-UnionFS Docker Image ===" && \
    echo "Available commands:" && \
    echo "  make test                - Run automated test suite" && \
    echo "  make clean               - Clean build artifacts" && \
    echo "  make install             - Show mount instructions" && \
    echo "  ./mini_unionfs           - Run the FUSE driver" && \
    echo "" && \
    echo "Example usage:" && \
    echo "  docker run -it --cap-add SYS_ADMIN --device /dev/fuse \\" && \
    echo "    -v /tmp/lower:/mnt/lower \\" && \
    echo "    -v /tmp/upper:/mnt/upper \\" && \
    echo "    unionfs /bin/bash"

# Default command: show help and start bash
CMD ["/bin/bash"]
