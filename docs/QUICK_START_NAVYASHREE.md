# 🚀 Path Resolution Implementation - Quick Start Guide
## For: Navyashree | Branch: `feature/path-resolution-layer-logic`

---

## ✅ What You Have

Your feature branch now contains:

### Code Modules (Ready for Integration)
```
src/
├── path_resolution.h       (Interface - read this first to understand functions)
├── path_resolution.c       (Core implementation - path resolution algorithm)
├── directory_merge.h       (Interface - getattr & readdir declarations)
└── directory_merge.c       (Implementation - merged directory logic)
```

### Documentation
```
docs/
├── TASK_PATH_RESOLUTION.md (Your detailed task breakdown - READ THIS!)
└── DESIGN.md               (System-wide design document)
```

### Team Coordination
```
README.md                   (Collaboration guide for all 4 team members)
```

---

## 📋 What These Modules Do

### path_resolution.c
**Purpose:** Determine where files actually live

**Key function:** `resolve_path(path, resolved_path)`
```
Input:  "/config.txt"
Output: "/path/to/upper/config.txt"  (and RESOLVE_IN_UPPER status)
OR:     "/path/to/lower/config.txt"  (and RESOLVE_IN_LOWER status)
OR:     error (and RESOLVE_WHITEOUTED or RESOLVE_NOT_FOUND)
```

**Algorithm:**
1. Check if file is whiteouted (deleted) → return WHITEOUTED
2. Check if file in upper directory → return UPPER
3. Check if file in lower directory → return LOWER
4. File doesn't exist → return NOT_FOUND

### directory_merge.c
**Purpose:** Show user a merged view of both layers

**Two key functions:**

1. **unionfs_getattr()** - Get file attributes
   - Calls resolve_path() to find the file
   - Calls lstat() to get metadata
   - Returns attributes to FUSE

2. **unionfs_readdir()** - List directory contents
   - Merges files from upper AND lower directories
   - Removes duplicates (upper takes precedence)
   - Hides whiteout files
   - User sees clean list

---

## 🎯 Your Immediate Next Steps

### Step 1: Understand the Code (15 min)
```bash
# Read the task document specific to your work
cat docs/TASK_PATH_RESOLUTION.md

# Read the algorithm definitions
cat src/path_resolution.h    # Comments explain each function
cat src/directory_merge.h    # Interface for the merged view
```

### Step 2: Test Compilation (5 min)
```bash
# Build the project
make clean && make

# Should compile with no errors or warnings
# (You may see linker warnings about undefined references - that's OK
#  because main mini_unionfs.c and other modules don't use your code yet)
```

### Step 3: Run Automated Tests (5 min)
```bash
# Run the test suite
make test

# Expected result: Some tests pass once modules are integrated
```

### Step 4: Integrate Your Code into Main Driver (30 min)
The old mini_unionfs.c has duplicate implementations. You need to:

**Action:** Update [src/mini_unionfs.c](src/mini_unionfs.c) to use your new modules

Replace these old functions:
```c
// DELETE FROM mini_unionfs.c:
- resolve_path()              // Use from path_resolution.c
- get_whiteout_name()         // Use from path_resolution.c
- is_whiteouted()             // Use from path_resolution.c
- unionfs_getattr()           // Use from directory_merge.c
- unionfs_readdir()           // Use from directory_merge.c
```

With includes:
```c
// ADD TO TOP of mini_unionfs.c:
#include "path_resolution.h"
#include "directory_merge.h"
```

Update Makefile:
```makefile
# CHANGE:
SOURCES = src/mini_unionfs.c

# TO:
SOURCES = src/mini_unionfs.c src/path_resolution.c src/directory_merge.c
```

### Step 5: Test Integration (10 min)
```bash
make clean && make
make test

# All tests should pass if integration is correct
```

---

## 📚 Understanding the Code

### Example: How resolve_path() Works

**Scenario:** User reads `/etc/config.txt`

```
Lower (read-only):          Upper (read-write):
├── etc/                    ├── (empty)
│   └── config.txt          │
│       "original"          │
└── base.txt                └── .wh.base.txt
    "original"                  (whiteout marker)
```

**When resolve_path("/etc/config.txt") is called:**

```
Step 1: Is /etc/config.txt whiteouted?
   Check: upper/etc/.wh.config.txt exists? NO → continue

Step 2: Does /etc/config.txt exist in upper?
   Check: upper/etc/config.txt exists? NO → continue

Step 3: Does /etc/config.txt exist in lower?
   Check: lower/etc/config.txt exists? YES
   → Return: RESOLVE_IN_LOWER, resolved_path = "/path/to/lower/etc/config.txt"
```

**Result:** User gets file from lower directory ✓

---

### Example: How readdir() Works

**Scenario:** User lists `/` directory

```
Lower:                  Upper:
├── base.txt            ├── modified.txt
├── to_delete.txt       └── .wh.to_delete.txt
└── unchanged.txt
```

**When readdir("/") is called:**

```
Phase 1: List upper directory
  Files: [modified.txt]
  Skip: [.wh.to_delete.txt]  (whiteout files are hidden)
  Result: [modified.txt]

Phase 2: List lower directory
  For each file in lower:
    - base.txt: not in upper? not whiteouted? YES → add
    - to_delete.txt: is whiteouted? YES → skip (hidden)
    - unchanged.txt: not in upper? not whiteouted? YES → add
  Result: [base.txt, unchanged.txt]

Final: Merge results
  Result: [modified.txt, base.txt, unchanged.txt]
```

**What user sees:** `ls /` outputs three files (clean merged view) ✓

---

## 🔍 Key Concepts

### Resolution Precedence (CRITICAL!)
```
Priority 1: Whiteouted? (Is it deleted?)
Priority 2: In upper?  (User's modifications)
Priority 3: In lower?  (Base image)
Priority 4: Not found
```

This ensures user always sees the "correct" version.

### Whiteout Files as Markers
- `.wh.filename` is NOT a real file for users
- It's an INTERNAL MARKER that says "filename is deleted"
- `readdir()` must filter them out
- `resolve_path()` checks for them but user never sees them

### Upper Always Wins
If file exists in both layers, upper version is used. This is correct because:
- User modifications are in upper
- Base is read-only
- User's changes should be visible

---

## 🧪 Manual Testing After Integration

```bash
# Create test environment
mkdir -p /tmp/test/{lower,upper,mnt}

# Create base files
echo "original content" > /tmp/test/lower/file.txt
echo "base file" > /tmp/test/lower/base.txt

# Mount filesystem
./mini_unionfs /tmp/test/lower /tmp/test/upper /tmp/test/mnt

# Test 1: Can see lower files?
ls -la /tmp/test/mnt/
# Should show: file.txt, base.txt

# Test 2: Attributes correct?
stat /tmp/test/mnt/file.txt
# Should match original in lower

# Clean up
fusermount -u /tmp/test/mnt
```

---

## 📊 Status Checklist

- [x] Feature branch created
- [x] Path resolution module implemented
- [x] Directory merge module implemented
- [x] Detailed task documentation created
- [x] Team collaboration guide written
- [ ] Code integrated into mini_unionfs.c (YOU DO THIS)
- [ ] All tests passing
- [ ] Code reviewed by team
- [ ] Merged to main branch

---

## 🤔 Debugging Tips

### Issue: "undefined reference to resolve_path"
**Solution:** Makefile not compiling path_resolution.c
```bash
# Check Makefile SOURCES line includes:
# SOURCES = src/mini_unionfs.c src/path_resolution.c src/directory_merge.c
```

### Issue: Tests fail with permission denied
**Solution:** Need root for FUSE operations
```bash
# Run with sudo
sudo make test
```

### Issue: Whiteout files visible to user
**Solution:** readdir() not filtering them
```c
// Check in unionfs_readdir:
if (strncmp(de->d_name, WHITEOUT_PREFIX, strlen(WHITEOUT_PREFIX)) == 0) {
    continue;  // Must skip whiteout files
}
```

### Issue: resolve_path returns wrong layer
**Solution:** Check stat() calls before strcmp()
```c
// path_resolution.c should use stat() to verify existence
if (stat(upper_path, &st) == 0) {
    // File definitely exists
    strcpy(resolved_path, upper_path);
    return RESOLVE_IN_UPPER;
}
```

---

## 📞 Need Help?

1. **Understanding the algorithm?** → Read `docs/TASK_PATH_RESOLUTION.md`
2. **Code not compiling?** → Check Makefile SOURCES variable
3. **Tests failing?** → Manual test with mkdir/touch to understand behavior
4. **Architecture question?** → Check `docs/DESIGN.md`

---

## 🎓 Learning Resources in Your Package

- **Algorithm explanation**: `docs/TASK_PATH_RESOLUTION.md` (with walkthroughs)
- **Code comments**: Every function in `.h` files has detailed comments
- **Design rationale**: `docs/DESIGN.md` section on path resolution
- **Examples**: Walkthrough section in task document

---

## ✨ Ready to Code!

You have everything you need. The implementation is:
- ✅ Well-documented
- ✅ Thoroughly commented
- ✅ Ready for integration
- ✅ Fully modular (clean separation from rest of system)

**Next branch to coordinate with:**
- **Team Member 2** will work on file I/O (reads/writes) using your resolve_path()
- **Team Member 3** will work on copy-on-write using your path resolution
- **Team Member 4** will work on deletions using your whiteout logic

Your code is the foundation. Make it solid! 💪

---

**Document:** Path Resolution Quick Start  
**Branch:** `feature/path-resolution-layer-logic`  
**Status:** Ready for implementation and integration
