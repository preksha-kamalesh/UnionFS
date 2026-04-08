# Docker Setup for Mini-UnionFS
## Cross-Platform Development (Windows, MacOS, Linux)

---

## 🎯 Why Docker?

Your team uses different operating systems:
- 1 team member: **MacOS**
- 3 team members: **Windows**

Docker ensures **everyone** gets the same development environment (Ubuntu 22.04 with FUSE support) regardless of their OS.

**Benefits:**
- ✅ Same build system for everyone
- ✅ No "but it works on my machine" problems
- ✅ FUSE works properly (required for filesystem)
- ✅ Easy to test and reproduce issues
- ✅ Professional deployment practice

---

## 📋 Prerequisites

### For Windows Users
1. **Install Docker Desktop for Windows**
   - https://www.docker.com/products/docker-desktop
   - Includes Docker CLI and docker-compose
   - Requires Windows 10 Home or Pro

2. **Enable WSL 2 (Windows Subsystem for Linux 2)**
   - Docker Desktop will guide you through this
   - Required for full FUSE support

3. **Verify Installation**
   ```powershell
   docker --version
   docker-compose --version
   ```

### For MacOS Users
1. **Install Docker Desktop for Mac**
   - https://www.docker.com/products/docker-desktop
   - Native macOS installation with ARM support (M1/M2)

2. **Verify Installation**
   ```bash
   docker --version
   docker-compose --version
   ```

### For Linux Users
1. **Install Docker & Docker Compose**
   ```bash
   sudo apt-get install docker.io docker-compose
   sudo usermod -aG docker $USER
   newgrp docker
   ```

2. **Verify Installation**
   ```bash
   docker --version
   docker-compose --version
   ```

---

## 🚀 Quick Start (For Everyone)

### Option A: Using Docker Compose (Recommended)

```bash
# Navigate to project directory
cd /path/to/UnionFS

# Start the container
docker-compose up -d

# Enter the container
docker-compose exec unionfs /bin/bash

# Inside container - compile
make clean && make

# Run tests
make test

# When done - exit container
exit

# Stop the container
docker-compose down
```

### Option B: Using Docker CLI Only

```bash
# Build the Docker image
docker build -t mini-unionfs .

# Run the container
docker run -it \
  --cap-add SYS_ADMIN \
  --device /dev/fuse \
  -v $(pwd):/unionfs \
  mini-unionfs

# Inside container - compile and test
make clean && make
make test
```

---

## 📖 Common Commands

### Build Commands
```bash
# Build image from Dockerfile
docker build -t mini-unionfs .

# Build with docker-compose
docker-compose build
```

### Run Commands
```bash
# Interactive shell with docker-compose
docker-compose up -d
docker-compose exec unionfs /bin/bash

# One-off command with docker-compose
docker-compose exec unionfs make test

# Interactive shell with docker CLI
docker run -it --cap-add SYS_ADMIN --device /dev/fuse \
  -v $(pwd):/unionfs mini-unionfs /bin/bash
```

### Stop & Cleanup
```bash
# Stop container (docker-compose)
docker-compose down

# Stop container (docker CLI)
docker stop <container-id>

# Remove container
docker rm <container-id>

# Remove image
docker rmi mini-unionfs
```

### View Container Info
```bash
# List running containers
docker ps

# List all containers
docker ps -a

# View container logs
docker logs <container-id>

# View container stats
docker stats
```

---

## 🧪 Testing Inside Docker

### Run Automated Tests
```bash
# Inside container
make test

# Expected output:
# Test 1: Layer Visibility... PASSED
# Test 2: Copy-on-Write... PASSED
# Test 3: Whiteout mechanism... PASSED
# Test 4: File Creation... PASSED
# Test 5: Directory Listing... PASSED
# Test 6: Directory Creation... PASSED
```

### Manual Testing
```bash
# Inside container, create test directories
mkdir -p /tmp/{lower,upper,mnt}
echo "test content" > /tmp/lower/file.txt

# Mount the filesystem
./mini_unionfs /tmp/lower /tmp/upper /tmp/mnt

# In another terminal (docker-compose exec unionfs bash)
# Test file visibility
ls -la /tmp/mnt/
cat /tmp/mnt/file.txt

# Unmount
fusermount -u /tmp/mnt
```

---

## 🔧 Troubleshooting

### Issue: "Cannot connect to Docker daemon"
**Windows/Mac:**
- Ensure Docker Desktop is running
- Restart Docker Desktop if needed

**Linux:**
```bash
sudo systemctl start docker
sudo usermod -aG docker $USER
# Logout and login for group changes to take effect
```

### Issue: Permission Denied for FUSE
**Solution:** Docker container already has required capabilities set in docker-compose.yml

If using `docker run` directly, add:
```bash
docker run -it \
  --cap-add SYS_ADMIN \
  --device /dev/fuse \
  mini-unionfs /bin/bash
```

### Issue: Port/Device Already in Use
```bash
# Stop all containers
docker-compose down

# Or for specific container
docker stop $(docker ps -q)
```

### Issue: "Docker daemon not responding" on Windows
- Check that WSL 2 backend is enabled
- Restart Windows
- Use Windows Event Viewer to check for error messages

---

## 📁 Docker Volumes Explained

### In docker-compose.yml
```yaml
volumes:
  - .:/unionfs              # Project code
  - ./test_data/lower:/tmp/lower    # Test lower layer
  - ./test_data/upper:/tmp/upper    # Test upper layer
  - ./test_data/mnt:/tmp/mnt        # Mount point
```

**How it works:**
- Left side = path on your **host machine**
- Right side = path inside **container**
- Changes in either location sync to the other

### Creating Test Directories
```bash
# On your host machine (outside container)
mkdir -p ./test_data/{lower,upper,mnt}
echo "base content" > ./test_data/lower/file.txt

# Inside container
cat /tmp/lower/file.txt  # Same file!
```

---

## 🌍 Team Workflow with Docker

### For MacOS Team Member
```bash
# Clone repository
git clone <repo-url>
cd UnionFS

# Start development environment
docker-compose up -d
docker-compose exec unionfs bash

# Inside container - work as normal
git status
make clean && make
make test
git add -A && git commit -m "..."
git push origin feature/branch-name
```

### For Windows Team Members
```powershell
# Clone repository
git clone <repo-url>
cd UnionFS

# Start development environment
docker-compose up -d

# Enter container
docker-compose exec unionfs /bin/bash

# Inside container - work as normal
make clean && make
make test
# ... same as MacOS ...
```

### Key Points
- ✅ All team members use Ubuntu 22.04 inside container
- ✅ Git works inside and outside container (shared via volume)
- ✅ Code changes sync automatically
- ✅ Tests run identically for all team members

---

## 🔍 What's in the Image?

The Docker image automatically includes:
- ✅ Ubuntu 22.04
- ✅ GCC compiler
- ✅ Make build system
- ✅ FUSE development headers
- ✅ Git
- ✅ Text editors (vim, nano)
- ✅ Your Mini-UnionFS code (pre-compiled)

The image is **~500MB** (reasonable for a development image).

---

## 🎓 Docker Best Practices We're Using

1. **Multi-stage builds** (room for improvement if size matters)
2. **Non-root user** (unionfs user for security)
3. **.dockerignore** (keeps images lean)
4. **Volume mounts** (live code editing)
5. **Capability restrictions** (only SYS_ADMIN for FUSE)
6. **Health checks** (room for expansion)

---

## 📊 Comparison: Before vs After Docker

### Before Docker
```
Windows User:
- Install WSL
- Install build tools
- Install FUSE headers
- Hope everything works
- Someone always has issues

MacOS User:
- Install Xcode
- Install Homebrew
- Install libfuse
- Different paths/versions
- Different test results

Linux User:
- Usually fine
- But dependencies vary
```

### After Docker
```
All Users:
- Install Docker Desktop
- docker-compose up -d
- docker-compose exec unionfs bash
- make test
- ✅ Tests pass identically!
```

---

## 🚀 Deployment Ready

Docker also prepares you for:
- **CI/CD Pipelines** - GitHub Actions, GitLab CI
- **Container Registry** - Push image to Docker Hub/registry
- **Orchestration** - Kubernetes, Docker Swarm
- **Production** - Same image runs everywhere

---

## 📝 Next Steps

1. **Install Docker Desktop**
   - Windows: https://docs.docker.com/desktop/install/windows-install/
   - MacOS: https://docs.docker.com/desktop/install/mac-install/
   - Linux: https://docs.docker.com/engine/install/ubuntu/

2. **Verify Installation**
   ```bash
   docker --version
   docker-compose --version
   ```

3. **Start Development**
   ```bash
   cd UnionFS
   docker-compose up -d
   docker-compose exec unionfs bash
   make test
   ```

4. **Share with Team**
   - Commit Dockerfile and docker-compose.yml to Git
   - Send this guide to team members
   - They can get the same environment in 5 minutes!

---

## 📞 Need Help?

### Docker Documentation
- **Official Docs:** https://docs.docker.com/
- **Docker Compose Guide:** https://docs.docker.com/compose/
- **Docker Best Practices:** https://docs.docker.com/dev-best-practices/

### Troubleshooting
- Check Docker logs: `docker logs <container-id>`
- Rebuild clean: `docker-compose down && docker-compose up --build`
- Check disk space: `docker system df`

---

## ✨ Summary

Docker gives your team:
- ✅ **Consistency** - Same environment for all
- ✅ **Reliability** - No "works on my machine" problems
- ✅ **Simplicity** - One command to get started
- ✅ **Scalability** - Easy to add more team members
- ✅ **Professionalism** - Industry-standard practice

**Everyone has the same Linux environment. Tests pass identically. Problems are reproducible. Ship it! 🚀**

---

**Document Version:** 1.0  
**Date:** March 12, 2026  
**Status:** Ready for team use
