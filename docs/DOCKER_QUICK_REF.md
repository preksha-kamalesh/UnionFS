# Docker Quick Reference
## Copy-Paste Commands for Your OS

---

## 💻 WINDOWS Users

### First Time Setup
```powershell
# 1. Install Docker Desktop from https://www.docker.com/products/docker-desktop
# 2. Verify installation
docker --version

# 3. Navigate to project
cd C:\Users\YourName\Desktop\UnionFS

# 4. Start development container
docker-compose up -d

# 5. Enter container
docker-compose exec unionfs /bin/bash
```

### Daily Development
```powershell
# Start container (if not running)
docker-compose up -d

# Enter container
docker-compose exec unionfs /bin/bash

# Inside container:
make clean && make
make test

# Stop when done
docker-compose down
```

### One-Liners
```powershell
# Build and test in one go
docker-compose up -d && docker-compose exec unionfs "make clean && make && make test"

# Rebuild image from scratch
docker-compose down && docker-compose build --no-cache && docker-compose up -d

# View container logs
docker-compose logs -f unionfs
```

---

## 🍎 MACOS Users

### First Time Setup
```bash
# 1. Install Docker Desktop from https://www.docker.com/products/docker-desktop
# 2. Verify installation
docker --version

# 3. Navigate to project
cd ~/Desktop/UnionFS

# 4. Start development container
docker-compose up -d

# 5. Enter container
docker-compose exec unionfs /bin/bash
```

### Daily Development
```bash
# Start container (if not running)
docker-compose up -d

# Enter container
docker-compose exec unionfs /bin/bash

# Inside container:
make clean && make
make test

# Stop when done
docker-compose down
```

### One-Liners
```bash
# Build and test in one go
docker-compose up -d && docker-compose exec unionfs "make clean && make && make test"

# Rebuild image from scratch
docker-compose down && docker-compose build --no-cache && docker-compose up -d

# View container logs
docker-compose logs -f unionfs
```

---

## 🐧 LINUX Users

### First Time Setup
```bash
# 1. Install Docker
sudo apt-get update
sudo apt-get install docker.io docker-compose
sudo usermod -aG docker $USER
newgrp docker

# 2. Verify installation
docker --version

# 3. Navigate to project
cd ~/UnionFS

# 4. Start development container
docker-compose up -d

# 5. Enter container
docker-compose exec unionfs /bin/bash
```

### Daily Development
```bash
# Start container (if not running)
docker-compose up -d

# Enter container
docker-compose exec unionfs /bin/bash

# Inside container:
make clean && make
make test

# Stop when done
docker-compose down
```

### One-Liners
```bash
# Build and test in one go
docker-compose up -d && docker-compose exec unionfs "make clean && make && make test"

# Rebuild image from scratch
docker-compose down && docker-compose build --no-cache && docker-compose up -d

# View container logs
docker-compose logs -f unionfs

# Cleanup unused images and containers
docker system prune
```

---

## 🧪 Testing Commands (Inside Container)

```bash
# Run automated test suite
make test

# Compile only
make clean && make

# Create test directories
mkdir -p /tmp/{lower,upper,mnt}
echo "test" > /tmp/lower/file.txt

# Mount filesystem
./mini_unionfs /tmp/lower /tmp/upper /tmp/mnt

# View merged directory
ls -la /tmp/mnt/

# Unmount
fusermount -u /tmp/mnt
```

---

## 🔧 Container Management

### View Status
```bash
# List running containers
docker ps

# View container logs
docker logs <container-id>
docker-compose logs -f

# View container stats
docker stats
```

### Stop/Start
```bash
# Stop container
docker-compose stop

# Start container
docker-compose start

# Restart container
docker-compose restart

# Stop and remove
docker-compose down
```

### Troubleshooting
```bash
# Rebuild from scratch
docker-compose build --no-cache

# Force recreate
docker-compose up -d --force-recreate

# Check disk usage
docker system df

# Cleanup everything
docker system prune -a
```

---

## 🐛 Common Issues & Fixes

### Docker Desktop won't start (Windows)
```powershell
# Restart Docker Desktop
# If that doesn't work:
# 1. Press Win+R, type "services.msc"
# 2. Find "Docker Desktop Service"
# 3. Right-click → Restart
```

### Permission denied (Linux)
```bash
# Add user to docker group
sudo usermod -aG docker $USER
newgrp docker

# Restart terminal/shell
exit
# Re-open terminal
```

### Container can't access FUSE
```bash
# The docker-compose.yml already has correct settings
# But if issues persist, restart Docker:
docker-compose down
docker-compose up -d
```

### Port/Device in use
```bash
# Stop all containers
docker-compose down
docker ps -a  # Should be empty

# Or kill specific container
docker kill <container-id>
```

---

## 📊 First Time: Expected Output

When you run `make test` inside container, you should see:

```
Starting Mini-UnionFS Test Suite...

Setting up test environment...
Mounting Mini-UnionFS...

Running Tests:

Test 1: Layer Visibility... PASSED
Test 2: Copy-on-Write... PASSED
Test 3: Whiteout Mechanism... PASSED
Test 4: File Creation... PASSED
Test 5: Directory Listing... PASSED
Test 6: Directory Creation... PASSED

Test Summary:
Passed: 6
Failed: 0

All tests passed!
```

**If you see all PASSED ✓ - You're good to go!**

---

## 🚀 Workflow

### Monday (Start of Sprint)
```bash
# Get latest code
docker-compose up -d
docker-compose exec unionfs git pull origin main

# Make sure tests still pass
docker-compose exec unionfs make test
```

### During Week (Different Branches)
```bash
# Create your feature branch
docker-compose exec unionfs git checkout -b feature/your-feature

# Work on your code (edits sync automatically)
# Test frequently
docker-compose exec unionfs make test

# Commit and push
docker-compose exec unionfs git push origin feature/your-feature
```

### End of Week (PR and Merge)
```bash
# Create Pull Request on GitHub/GitLab

# After approval, merge to main
docker-compose exec unionfs git checkout main
docker-compose exec unionfs git pull origin main
docker-compose exec unionfs make test  # Verify merge
```

---

## ✨ Tips & Tricks

### Keep container running in background
```bash
docker-compose up -d
# Container stays running - reconnect anytime with:
docker-compose exec unionfs bash
```

### Multiple terminals in same container
```bash
# Terminal 1: One task
docker-compose exec unionfs vim src/file.c

# Terminal 2: Another task (same container)
docker-compose exec unionfs make test

# Both work simultaneously!
```

### Share Docker stats with team
```bash
docker stats --no-stream
```

### Export test results
```bash
docker-compose exec unionfs make test | tee test_results.txt
# Share test_results.txt with team
```

---

## ❓ FAQ

**Q: Will Docker slow down my development?**
A: No - Docker runs natively on modern systems. Overhead is minimal.

**Q: Can I use VSCode inside Docker?**
A: Yes! VSCode has Remote Container extension. Install and it works seamlessly.

**Q: What if I need a different package?**
A: Edit Dockerfile, add the package to apt-get, rebuild: `docker-compose build`

**Q: Can I push the Docker image to Docker Hub?**
A: Yes! For team sharing: `docker tag mini-unionfs myteam/mini-unionfs && docker push ...`

**Q: Does Docker work offline?**
A: Yes, after initial build. Image is local, doesn't need internet.

---

## 🎯 Remember

- **Everyone uses the same image** ✓
- **Tests pass identically** ✓
- **No "works on my machine" problems** ✓
- **Professional setup** ✓

---

**Happy coding! 🚀**
