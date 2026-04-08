# Path Resolution & Layer Logic Implementation
## Task Specification for Navyashree

### Overview
Your responsibility is to implement the **path resolution layer** and **merged directory logic** for the Mini-UnionFS project. This is the core of how the file system determines which files to show users.

---

## What You're Building

When a user runs `ls mnt/`, they should see a **merged view** of files from both layers:
- Files from the read-write upper layer (container changes)
- Files from the read-only lower layer (base image)
- Deleted files should be hidden (via whiteouts)

Your code makes this magic work.

---

## File Structure

The code is organized into focused modules:

### 1. **path_resolution.h / path_resolution.c** ✅
Core path resolution logic

**Key Functions:**
- `construct_path()` - Build absolute paths
- `get_whiteout_name()` - Generate whiteout filenames
- `is_whiteouted()` - Check if file is deleted
- `resolve_path()` - **THE MAIN FUNCTION** - determines where a file is located

**Algorithm:**
```
resolve_path(path):
  1. Check if .wh.<filename> exists in upper_dir
     → YES: Return RESOLVE_WHITEOUTED (file is deleted)
     → NO: Continue to step 2
  
  2. Check if file exists in upper_dir
     → YES: Return RESOLVE_IN_UPPER (use this version)
     → NO: Continue to step 3
  
  3. Check if file exists in lower_dir
     → YES: Return RESOLVE_IN_LOWER (use this version)
     → NO: Continue to step 4
  
  4. File doesn't exist anywhere
     → Return RESOLVE_NOT_FOUND (return -ENOENT to user)
```

### 2. **directory_merge.h / directory_merge.c** ✅
Merged view logic for files and directories

**Key Functions:**
- `unionfs_getattr()` - Get file attributes (uses path resolution)
- `unionfs_readdir()` - List merged directory contents

**getattr() Algorithm:**
```
unionfs_getattr(path):
  1. Resolve path to actual location
  2. If whiteouted or not found:
     → Return -ENOENT (pretend file doesn't exist)
  3. If found in upper or lower:
     → Call lstat() on resolved path
     → Return file's attributes (size, permissions, etc.)
```

**readdir() Algorithm:**
```
unionfs_readdir(path):
  1. Track files = []
  
  2. List upper directory:
     FOR EACH file in upper_dir/path:
       - Skip if it's a whiteout file (.wh.*)
       - Add to results
       - Remember in "track files"
  
  3. List lower directory:
     FOR EACH file in lower_dir/path:
       - Skip if already in upper (duplicate)
       - Skip if whiteouted (hidden)
       - Add to results
  
  4. Return merged list to user
```

---

## Example Walkthrough

**Initial State:**
```
lower_dir/
  ├── file1.txt    ("original content")
  ├── file2.txt    ("will be modified")
  └── file3.txt    ("will be deleted")

upper_dir/
  └── (empty)
```

**After user modifies file2.txt:**
```
lower_dir/
  ├── file1.txt    ("original content")      [unchanged]
  ├── file2.txt    ("original content")      [unchanged]
  └── file3.txt    ("will be deleted")

upper_dir/
  └── file2.txt    ("modified content")      [copy-on-write]
```

**When user does `ls mnt/`:**
1. resolve_path("file1.txt") → RESOLVE_IN_LOWER → show file1.txt from lower
2. resolve_path("file2.txt") → RESOLVE_IN_UPPER → show file2.txt from upper (modified)
3. resolve_path("file3.txt") → RESOLVE_IN_LOWER → show file3.txt from lower

**After user deletes file3.txt:**
```
upper_dir/
  ├── file2.txt    ("modified content")
  └── .wh.file3.txt     [whiteout marker]
```

**When user does `ls mnt/`:**
1. resolve_path("file1.txt") → RESOLVE_IN_LOWER → show file1.txt
2. resolve_path("file2.txt") → RESOLVE_IN_UPPER → show file2.txt
3. resolve_path("file3.txt") → RESOLVE_WHITEOUTED → **hidden from user** ✓

---

## Testing Your Implementation

### Unit Test Commands
```bash
# Build the project
make clean && make

# Run the automated test suite
make test
```

### Manual Testing
```bash
# Create test directories
mkdir -p /tmp/lower /tmp/upper /tmp/mnt
echo "base file" > /tmp/lower/file.txt

# Mount your filesystem
./mini_unionfs /tmp/lower /tmp/upper /tmp/mnt

# List merged view
ls -la /tmp/mnt/

# Test file visibility
cat /tmp/mnt/file.txt

# Test copy-on-write
echo "modified" >> /tmp/mnt/file.txt
cat /tmp/mnt/file.txt     # Should see modification
cat /tmp/lower/file.txt   # Should NOT see modification
cat /tmp/upper/file.txt   # Should see modification

# Test whiteout
rm /tmp/mnt/file.txt      # Creates .wh.file.txt in upper
ls /tmp/mnt/              # Should not show file.txt
ls /tmp/upper/            # Should show .wh.file.txt

# Unmount
fusermount -u /tmp/mnt
```

---

## Implementation Checklist

### Phase 1: Path Resolution ✅
- [x] Create path_resolution.h
- [x] Implement construct_path()
- [x] Implement get_whiteout_name()
- [x] Implement is_whiteouted()
- [x] Implement resolve_path() with proper precedence

### Phase 2: File Attributes
- [ ] Implement unionfs_getattr()
- [ ] Handle all return codes from resolve_path()
- [ ] Test with `stat` command

### Phase 3: Directory Merging
- [ ] Implement unionfs_readdir()
- [ ] Handle upper directory listing
- [ ] Handle lower directory listing
- [ ] De-duplicate files
- [ ] Filter whiteout files
- [ ] Test with `ls` command

### Phase 4: Integration
- [ ] Update main mini_unionfs.c to use new functions
- [ ] Update Makefile to compile new files
- [ ] Run automated test suite
- [ ] All tests passing ✓

---

## Code Integration Notes

The new modular code integrates into the main file like this:

```c
// In mini_unionfs.c, replace old functions with:
#include "path_resolution.h"
#include "directory_merge.h"

static struct fuse_operations unionfs_oper = {
    .getattr = unionfs_getattr,      // from directory_merge.c
    .readdir = unionfs_readdir,      // from directory_merge.c
    // ... other operations ...
};
```

Update the Makefile:
```makefile
SOURCES = src/mini_unionfs.c src/path_resolution.c src/directory_merge.c
```

---

## Common Pitfalls & Solutions

### Pitfall 1: Path Construction Errors
**Problem:** Paths have double slashes or missing slashes
**Solution:** Always handle the leading '/' carefully in construct_path()

### Pitfall 2: Whiteout Not Found
**Problem:** .wh. files created but is_whiteouted() returns 0
**Solution:** Ensure get_whiteout_name() generates correct paths

### Pitfall 3: Duplicate Files in Directory Listing
**Problem:** readdir() shows same file twice
**Solution:** Track upper files and skip them when listing lower

### Pitfall 4: Upper Directory Not Existing
**Problem:** opendir() fails on non-existent upper directory
**Solution:** Check if directory exists before opening; handle gracefully

---

## Key Concepts

### Layer Precedence
Upper layer **always** takes precedence over lower layer. This ensures user modifications are always visible.

### Whiteouts as Markers
Whiteout files (`.wh.filename`) are **not** real files—they're markers that mean "this file is deleted."

### Immutable Lower Layer
The lower directory should never be modified by your code. Only reads allowed. All writes go to upper.

### Atomic Semantics
Each resolve_path() call must be consistent. The same path should resolve to the same location for the duration of an operation.

---

## Performance Notes

- `resolve_path()`: O(1) when upper exists, O(1) when lower exists, O(2) worst case
- `readdir()`: O(n + m) where n = upper files, m = lower files
- `getattr()`: O(1) – just a stat() call

For most operations, performance is optimal.

---

## Questions to Ask Before Starting

1. ❓ Do you understand the three-step resolution algorithm?
2. ❓ Can you trace through the whiteout example manually?
3. ❓ Do you know how to compile C with FUSE headers?
4. ❓ Can you run the automated tests?

---

## Success Criteria

✅ All 6 automated tests pass  
✅ `ls mnt/` shows merged file listing  
✅ File attributes correct (size, permissions)  
✅ Whiteout files invisible to user  
✅ Upper layer changes never affect lower  
✅ Code compiles with no warnings  

---

## Next Steps After This

Once path resolution is solid, other team members will build:
- **File reads/writes** (read, write operations)
- **Copy-on-Write** (open operation)
- **Deletion handling** (unlink, rmdir)

Your code is the foundation for all of them!

---

**Document Version:** 1.0  
**Branch:** feature/path-resolution-layer-logic  
**Status:** Ready for implementation
