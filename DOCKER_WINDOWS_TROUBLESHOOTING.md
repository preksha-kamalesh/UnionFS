# Docker Troubleshooting - Windows Specific
## Your Exact Error & Solution

---

## 🔴 Error You're Seeing

```
unable to get image 'mini-unionfs:latest': 
error during connect: Get "http://%2F%2F.%2Fpipe%2FdockerDesktopLinuxEngine": 
open //./pipe/dockerDesktopLinuxEngine: The system cannot find the file specified.
```

**Translation:** Docker Desktop is not running!

---

## ✅ Solution (Step by Step)

### Step 1: Start Docker Desktop

**Option A: Quick Start**
```powershell
# Just click the Docker Desktop icon on your desktop or taskbar
# Wait 30-60 seconds for it to fully load
# Check taskbar - you should see Docker icon moving/animating
```

**Option B: From Terminal**
```powershell
# If Docker Desktop shortcut doesn't exist:
Start-Process -FilePath "C:\Program Files\Docker\Docker\Docker.exe"
# Wait 30-60 seconds for startup
```

**Option C: Check Services (if needed)**
```powershell
# Press Win+R, type "services.msc", Enter
# Find "Docker Desktop Service"
# Right-click → Restart
# Close services window
```

### Step 2: Verify Docker is Running

```powershell
# Open new PowerShell terminal
docker ps
# If this shows a blank table with columns, Docker is running! ✓
# If it hangs or errors, Docker is still starting - wait more

# Check Docker status
docker info
# Should show system information
```

### Step 3: Build Docker Image

```powershell
# Now that Docker is running, build the image
cd C:\Users\NNR\Desktop\UnionFS
docker-compose build

# This will:
# 1. Download Ubuntu 22.04 base image
# 2. Install build tools
# 3. Install FUSE headers
# 4. Compile your code
# Takes 2-3 minutes first time
```

**Expected output:**
```
Building unionfs
Sending build context to Docker daemon
Step 1/12 : FROM ubuntu:22.04
22.04: Pulling from library/ubuntu
...
Successfully built mini-unionfs:latest
```

### Step 4: Start Container

```powershell
# Now start the container
docker-compose up -d

# Should see:
# creating unionfs-dev ... done
```

### Step 5: Enter Container & Test

```powershell
# Open bash shell inside container
docker-compose exec unionfs bash

# Now you're inside Ubuntu! Test:
make test

# Should see:
# Test 1: Layer Visibility... PASSED
# Test 2: Copy-on-Write... PASSED
# Test 3: Whiteout mechanism... PASSED
# Test 4: File Creation... PASSED
# Test 5: Directory Listing... PASSED
# Test 6: Directory Creation... PASSED
```

---

## 🔧 Quick Fixes

### Fix #1: Docker Desktop Keeps Disconnecting
```powershell
# Restart Docker
docker-compose down
docker system prune -f

# Try again
docker-compose up -d
```

### Fix #2: "version is obsolete" Warning
**Already fixed!** Updated docker-compose.yml to remove it.

### Fix #3: Still Can't Connect
```powershell
# Full nuclear option:
# 1. Quit Docker Desktop (right-click icon → Quit)
# 2. Wait 10 seconds
# 3. Delete all containers:
docker rm -f $(docker ps -aq)
# 4. Delete all images:
docker rmi -f $(docker images -q)
# 5. Restart Docker Desktop
# 6. Try again
```

---

## 📋 Step-by-Step Beginner Guide

### You're Currently At:
```
[STEP 1] ✓ Docker installed (docker --version works)
[STEP 2] ✗ Docker Desktop NOT RUNNING (this is the problem)
[STEP 3] NOT STARTED - docker-compose build
[STEP 4] NOT STARTED - docker-compose up -d
[STEP 5] NOT STARTED - docker-compose exec unionfs bash
[STEP 6] NOT STARTED - make test (inside container)
```

### To Complete:

#### IMMEDIATELY (Right now, takes 1 minute)
```powershell
# 1. Start Docker Desktop
# Click the Docker Desktop icon on your desktop
# You should see it in taskbar (whale icon)
# or in Start Menu > Docker Desktop

# 2. Verify it started
docker ps
# Should show empty table with columns

# 3. Now try:
docker-compose build
# This will start building - takes 2-3 min
```

#### AFTER BUILD (takes 3-5 minutes total)
```powershell
# Wait for build to finish, then:
docker-compose up -d
docker-compose exec unionfs bash
make test
# All tests should PASS! ✓
```

---

## 🎯 Why This Happens

**On Windows, Docker needs:**
1. **Docker Desktop application running** (the GUI)
2. **WSL 2 backend enabled** (Windows Subsystem for Linux)

When you installed Docker Desktop:
- ✓ Installed the Docker CLI command
- ✗ But application isn't auto-running

**Your `docker --version` works because:**
- Docker CLI is installed on Windows directly

**Your `docker-compose up` fails because:**
- It needs Docker Desktop service running to create containers

---

## 💡 Pro Tip

Add Docker Desktop to startup (won't auto-start, but easier to launch later):

```powershell
# Create shortcut in Startup folder
$source = "C:\Program Files\Docker\Docker\Docker.exe"
$destination = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup\Docker.lnk"

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($destination)
$Shortcut.TargetPath = $source
$Shortcut.Save()

echo "Docker shortcut created in Startup folder"
```

Then Docker will start when Windows boots (optional).

---

## ✨ Once Docker is Running

Everything becomes easy:

```powershell
# Daily workflow:
docker-compose up -d          # Start container
docker-compose exec unionfs bash   # Enter it
make clean && make            # Build  
make test                     # Test (6/6 pass)
git add -A && git commit      # Commit work
exit                          # Leave container
docker-compose down           # Stop it
```

---

## 🚨 Remember

🔴 **ERROR:** `unable to get image 'mini-unionfs:latest'`  
✅ **CAUSE:** Docker Desktop service not running  
✅ **FIX:** Click Docker Desktop icon and wait 30 seconds  
✅ **VERIFY:** `docker ps` shows empty table (not error)  

Once `docker ps` works without error, everything else will work!

---

## Need More Help?

Check these in order:
1. **Is Docker Desktop icon visible in taskbar?** If not → Start it
2. **Does `docker ps` work without errors?** If not → Docker still starting
3. **Does `docker-compose build` run?** If not → Still not running properly
4. **Did build finish successfully?** If yes → Try `docker-compose up -d`
5. **Can you enter container?** If yes → Try `make test`

---

**Key Point:** Your installation is fine. Docker Desktop just needs to be running!

After you start Docker Desktop and `docker ps` works, all the Docker commands will work perfectly.

---

**Status:** 
- Docker CLI: ✓ Working
- Docker Desktop Service: ❌ Not running (FIX THIS FIRST)
- Everything else: Will work once Docker Desktop runs
