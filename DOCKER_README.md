# Docker Setup for Cross-Platform Team
## Windows, MacOS, and Linux support via Docker

---

## 📢 IMPORTANT: Your Team's Situation

**Team Composition:**
- ✅ 1 team member using **MacOS**
- ✅ 3 team members using **Windows**
- ❌ Different operating systems = Different environments = Problems

**Before Docker:**
```
Windows User 1: Install WSL, FUSE headers, tools, etc.
Windows User 2: Install WSL, FUSE headers, tools, etc.
Windows User 3: Install WSL, FUSE headers, tools, etc.
MacOS User:    Install HomeBrew, FUSE, tools, etc.

Result: 4 different setups, 4 different potential issues
```

**With Docker:**
```
All Users: docker-compose up -d
Result: Everyone has identical Ubuntu 22.04 environment with FUSE!
```

---

## ✅ What You Now Have

### New Files Created

```
UnionFS/
├── Dockerfile              ← Build recipe for Docker image
├── docker-compose.yml      ← Orchestration for easy use
├── .dockerignore           ← Keep image lean
├── .github/
│   └── workflows/
│       └── ci.yml          ← Automated testing on every push
└── docs/
    ├── DOCKER_SETUP.md     ← Detailed setup guide
    └── DOCKER_QUICK_REF.md ← Copy-paste commands
```

### What Docker Provides

**Dockerfile includes:**
- ✅ Ubuntu 22.04 (latest LTS Linux)
- ✅ GCC compiler
- ✅ Make build system  
- ✅ FUSE development headers
- ✅ Git for version control
- ✅ Text editors (vim, nano)
- ✅ Your Mini-UnionFS code (pre-compiled)

**docker-compose.yml includes:**
- ✅ Easy container management
- ✅ Volume mounts for development
- ✅ FUSE capabilities configured
- ✅ Test directory setup
- ✅ Network configuration

**CI/CD Pipeline (.github/workflows/ci.yml):**
- ✅ Auto-test on every push
- ✅ Auto-test on PRs
- ✅ Code quality checks
- ✅ Documentation validation
- ✅ Building in Docker

---

## 🚀 Getting Started (All Team Members)

### Step 1: Install Docker (5 minutes)

**Windows:**
1. Download Docker Desktop: https://www.docker.com/products/docker-desktop
2. Run installer, follow prompts
3. Enable WSL 2 (Docker will guide you)
4. Restart computer when asked

**MacOS:**
1. Download Docker Desktop: https://www.docker.com/products/docker-desktop
2. Run installer, drag Docker to Applications
3. Launch Docker from Applications
4. Mac may prompt for password - approve it

**Linux:**
```bash
sudo apt-get update
sudo apt-get install docker.io docker-compose
sudo usermod -aG docker $USER
newgrp docker
```

### Step 2: Verify Installation

```bash
docker --version      # Should show: Docker version XX.XX.XX
docker-compose --version  # Should show: docker-compose version X.XX.X
```

### Step 3: Clone Project (if not already)

```bash
git clone <repository-url>
cd UnionFS
```

### Step 4: Start Development Environment

```bash
# Windows (PowerShell)
docker-compose up -d

# MacOS or Linux
docker-compose up -d

# Enter container
docker-compose exec unionfs /bin/bash
```

### Step 5: Test It Works

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

**If all 6 tests pass: ✅ YOU'RE DONE!**

---

## 💼 Daily Development Workflow

### Morning
```bash
# Navigate to project
cd ~/path/to/UnionFS  # All OSes

# Start the container
docker-compose up -d

# Enter the container
docker-compose exec unionfs /bin/bash

# Now you're inside Ubuntu 22.04 with everything you need
```

### During Day
```bash
# Edit code (outside container, in your IDE)
# VSCode, Sublime, IntelliJ, etc. all work

# Changes sync automatically to container

# Test frequently (inside container)
make test

# All tests pass? Commit and push!
git add -A
git commit -m "feat: implement feature"
git push origin feature/your-branch
```

### End of Day
```bash
# Stop the container
exit  # Leave container

docker-compose down  # Stop container
```

### Key Point
- You edit code in your IDE on **your operating system**
- The code syncs into **the Docker container**
- Tests run in the **identical Ubuntu environment** for everyone
- When tests pass for you, they pass for everyone ✓

---

## 📖 Read These Docs

| Document | Purpose | Read Time |
|----------|---------|-----------|
| **DOCKER_QUICK_REF.md** | Copy-paste commands for your OS | 5 min |
| **DOCKER_SETUP.md** | Detailed explanation of Docker | 15 min |
| **SETUP_COMPLETE.md** | Your current project status | 5 min |

---

## 🎯 Why This is Better

### Before Docker
```
Problem: "Tests pass on my machine but fail on GitHub Actions"
Reason: Different environment

Problem: "It works on MacOS but fails on Windows"
Reason: Different tools/libraries

Problem: "I installed the tools but I'm getting permission errors"
Reason: System-specific configuration
```

### With Docker
```
Solution: Everyone uses the same Docker container
Result: Tests pass identically for all team members

Solution: Docker image runs on all OSes
Result: Development works on Windows/MacOS/Linux equally

Solution: Dockerfile defines exactly what's needed
Result: No "install this tool" confusion - everything is automated
```

---

## 🔄 Team Coordination

### Scenario: Navyashree Pushes Code

1. **Navyashree (on Windows):**
   ```powershell
   docker-compose exec unionfs bash
   git add -A
   git commit -m "feat: implement path resolution"
   git push origin feature/path-resolution-layer-logic
   ```

2. **GitHub Actions (automatic):**
   - Builds Docker image
   - Runs `make test`
   - Comments on PR if tests fail
   - Marks PR as "ready to merge" if tests pass

3. **Team Member 2 (on MacOS):**
   ```bash
   docker-compose exec unionfs bash
   git pull origin feature/path-resolution-layer-logic
   make test
   # Same tests pass identically!
   ```

---

## 🧠 How Docker Works (Simple Version)

```
Docker Image = Recipe for an environment
Docker Container = Running instance of that environment

Dockerfile says:
  "Start with Ubuntu 22.04
   Install GCC, Make, FUSE headers
   Copy code into /unionfs
   Pre-compile the project"

docker-compose says:
  "Run this image
   Mount my code inside it
   Give it FUSE capabilities
   Keep it running in background"

Your IDE says:
  "I'm editing code on Windows/MacOS/Linux
   But it syncs into the container
   Tests run in the container
   Results come back to me"
```

---

## 🔧 Common Operations

### Start Work
```bash
docker-compose up -d
docker-compose exec unionfs bash
```

### Edit Code
```bash
# Use your normal IDE (VSCode, Sublime, etc.)
# Files sync automatically into container
```

### Test Your Code
```bash
# Inside container
make clean && make
make test
```

### Commit and Push
```bash
# Still inside container
git add -A
git commit -m "your message"
git push origin your-branch
```

### Stop Work
```bash
exit  # Leave container
docker-compose down  # Stop
```

---

## 🚨 Troubleshooting

### "Docker command not found"
- Did you restart after installing Docker?
- Restart your terminal/computer

### "Permission denied: fusermount"
- Docker setup already handles this
- Try: `docker-compose down && docker-compose up -d`

### "Cannot connect to Docker daemon"
- Is Docker Desktop running?
- (Windows) Check system tray icon
- (MacOS) Check top menu bar
- Restart Docker Desktop

### "FUSE not working in container"
- Our docker-compose.yml has the right settings
- Try rebuilding: `docker-compose build --no-cache`

---

## 🎓 Learning Resources

### For Understanding Docker Better
- **Official Docker Docs:** https://docs.docker.com/
- **Container Concepts:** https://www.docker.com/resources/what-container
- **FUSE in Docker:** Special handling in our setup (already done)

### For Your Specific Tools
- **GCC/Make:** https://www.gnu.org/software/gcc/
- **FUSE:** https://github.com/libfuse/libfuse
- **Ubuntu:** https://ubuntu.com/

---

## 📊 Project Status with Docker

### Before Docker Setup
```
✓ Path resolution code: Written
✓ Directory merge code: Written
✓ Documentation: Complete
✓ Tests: Created
✗ Team can run identically: NO (different OSes)
```

### After Docker Setup
```
✓ Path resolution code: Written
✓ Directory merge code: Written
✓ Documentation: Complete
✓ Tests: Created
✓ Team can run identically: YES! (Docker ensures it)
✓ CI/CD Pipeline: Automatic testing
✓ Professional setup: Industry standard
```

---

## ✨ Next Steps

1. **All Team Members:**
   - Install Docker Desktop
   - Verify: `docker --version`

2. **Clone/Pull Latest:**
   ```bash
   git pull origin main
   ```

3. **Start Development:**
   ```bash
   docker-compose up -d
   docker-compose exec unionfs bash
   make test  # Verify all passes
   ```

4. **Start Your Work:**
   - Each member creates feature branch
   - Uses docker-compose for development
   - Commits and pushes as normal
   - GitHub Actions auto-tests every push

---

## 🎉 Benefits Summary

| Aspect | Benefit |
|--------|---------|
| **Consistency** | Same environment for all 4 team members |
| **Simplicity** | `docker-compose up` - that's it |
| **Reliability** | Same results every time, every OS |
| **Professionalism** | Industry standard approach |
| **Learning** | You're learning containerization (Docker skill) |
| **Scalability** | Easy to onboard new team members |
| **Quality** | CI/CD auto-tests every change |

---

## 🚀 You're Ready!

Everything is set up. The Dockerfile is optimized. The docker-compose.yml is configured. The CI/CD pipeline is ready.

**All you need to do:**
1. Install Docker
2. `docker-compose up -d`
3. `docker-compose exec unionfs bash`
4. `make test`

That's it! Same environment, same results, same success. 🎉

---

**Document Version:** 1.0  
**Date:** March 12, 2026  
**Status:** Ready for team deployment
