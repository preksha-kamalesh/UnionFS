# Mini-UnionFS Design Document

## 1. Overview

Mini-UnionFS is a Linux userspace file system (FUSE) implementation that provides a unified view of layered directories. It merges read-only "lower" directories with a read-write "upper" directory, implementing Copy-on-Write (CoW) semantics and whiteout-based deletion tracking, mirroring the mechanisms used in Docker container filesystems.

## 2. Architecture

### 2.1 System Design

The system consists of three main components:

1. **Layered Directory Structure**:
   - `lower_dir`: Read-only base image layer(s)
   - `upper_dir`: Read-write container layer
   - `mount_point`: Virtual unified filesystem entry

2. **FUSE Interface**: Intercepts filesystem operations and routes them through custom handlers

3. **Path Resolution Engine**: Determines the actual location of files considering whiteouts and layer precedence

### 2.2 Core Data Structures

#### mini_unionfs_state
```c
struct mini_unionfs_state {
    char *lower_dir;    // Path to read-only lower directory
    char *upper_dir;    // Path to read-write upper directory
};
```

This global structure holds the mount configuration and is passed to all FUSE callbacks via `fuse_get_context()->private_data`.

#### Key Constants
- `MAX_PATH_LEN`: 4096 bytes for path buffers (supports standard Linux paths)
- `WHITEOUT_PREFIX`: ".wh." prefix for whiteout files

### 2.3 Path Resolution Algorithm

The `resolve_path()` function implements the layering precedence:

1. **Whiteout Check**: If `.wh.<filename>` exists in upper_dir, the file is deleted (return ENOENT)
2. **Upper Layer**: If file exists in upper_dir, use it (precedence)
3. **Lower Layer**: If file exists in lower_dir, use it
4. **Not Found**: Return -ENOENT

This ensures:
- Modifications take precedence over base versions
- Deletions hide base files without modifying them
- Consistent visibility across all operations

## 3. Core Operations

### 3.1 Read Operations

**getattr**: Path resolution → lstat() on resolved path
- Validates path existence and retrieves metadata
- Returns ENOENT for whiteouted files

**readdir**: Merges directory listings from both layers
- Upper directory files listed first (take precedence)
- Lower directory files added (excluding duplicates and whiteouted files)
- Prevents whiteout files from appearing in listings
- O(n) complexity, n = total files in both directories

**read**: Resolves path → open in read mode → pread() with offset
- Non-modifying, can safely read from either layer
- Offset-based reading for partial file access

### 3.2 Write Operations

**create**: Creates file exclusively in upper_dir
- Mode and permissions applied to new file
- Prevents accidental overwrites

**open**: Implements Copy-on-Write logic
- If opening lower_dir file with write flags (O_WRONLY, O_RDWR, O_APPEND):
  - Copy entire file to upper_dir using system `cp` command
  - Subsequent writes happen in upper_dir
  - Lower_dir remains unmodified
- Flags checked: `O_WRONLY | O_RDWR | O_APPEND`

**write**: Resolves path → pwrite() at specified offset
- Works on already-resolved (or CoW'd) file
- Maintains sparse file structure through pwrite

### 3.3 Deletion and Directory Operations

**unlink**: Implements whiteout mechanism
- If file in upper_dir: physically delete it
- If file in lower_dir: create `.wh.<filename>` in upper_dir
- Both operations may occur (file in both layers)

**mkdir**: Creates directory exclusively in upper_dir
- Directories are created fresh, not layered from below

**rmdir**: Removes directory with whiteout support
- Removes directory from upper_dir if present
- If directory exists in lower_dir, creates whiteout
- Prevents accidental recreation of base directories

## 4. Edge Cases and Handling

### 4.1 Copy-on-Write Scenarios

**Scenario**: User appends to base.txt (exists only in lower_dir)
- **Handler**: open() detects O_APPEND flag on lower_dir file
- **Action**: Copy entire file to upper_dir before write
- **Result**: base.txt now exists in both layers; writes affect upper_dir version

**Scenario**: User modifies a file that exists in both layers
- **Handler**: open() sees file already in upper_dir
- **Action**: No copy needed; write directly to upper_dir version
- **Result**: More efficient, no unnecessary copying

### 4.2 Whiteout Scenarios

**Scenario**: User deletes lower_dir file not yet in upper_dir
- **Handler**: unlink() detects file only in lower_dir
- **Action**: Create `.wh.<filename>` in upper_dir
- **Result**: File hidden in merged view; base remains untouched

**Scenario**: User deletes upper_dir file (not in lower_dir)
- **Handler**: unlink() detects file only in upper_dir
- **Action**: Physical delete; no whiteout needed
- **Result**: File completely removed from both views

**Scenario**: User deletes file that exists in both layers
- **Handler**: unlink() detects presence in both
- **Action**: Delete from upper_dir AND create whiteout
- **Result**: Complete removal from merged view; base hidden

### 4.3 Directory Scenarios

**Scenario**: User creates file in non-existent subdirectory
- **Handler**: mkdir() explicitly creates parent in upper_dir first
- **Action**: Hierarchical directory creation
- **Result**: New directory structure in upper_dir

**Scenario**: User lists directory existing in both layers
- **Handler**: readdir() merges listings smartly
- **Action**: Combine entries, remove duplicates, filter whiteouts
- **Result**: Single merged directory view

**Scenario**: Nested whiteouts (whiteout in subdirectory)
- **Handler**: readdir() recursively checks subdirectory whiteouts
- **Action**: Full path whiteout names preserve uniqueness
- **Result**: Correct hiding of nested deleted files

### 4.4 Concurrency and Consistency

**Assumption**: Single-threaded FUSE daemon (default configuration)
- No locking required for path resolution
- Atomic operations rely on OS filesystem atomicity
- File descriptor reuse prevented by OS

**Potential Race Condition**: CoW during concurrent access
- **Current Handling**: cp command is atomic for file contents
- **Improvement Needed**: Add fine-grained locking if multi-threaded

### 4.5 Filesystem Integrity

**Orphaned Whiteouts**: Whiteout exists but base file deleted
- **Impact**: No visible effect; ignored during readdir
- **Handling**: User can manually clean up via rm

**Incomplete CoW**: Copy-on-Write fails mid-operation
- **Recovery**: File ends up in inconsistent state
- **Improvement**: Atomic copy or transactional semantics

**Cross-Layer Hard Links**: Not supported
- **Reason**: Can't create hard links across mount boundaries
- **Degradation**: Link operations fail gracefully with EXDEV

## 5. Implementation Notes

### 5.1 Path Handling

All paths are resolved to absolute paths:
```
construct_path(upper_dir, "/file.txt", result)
→ /unionfs/mount/upper/.../file.txt
```

This prevents directory traversal attacks and ensures consistent resolution.

### 5.2 FUSE Configuration

- **FUSE_USE_VERSION**: 31 (stable API)
- **Debug Flags**: `-f` (foreground) and `-d` (debug output) enabled
- **Entry Caching**: Disabled for maximum correctness

### 5.3 Error Handling

All system calls checked; errors propagated as errno:
- `open()` failure → -errno
- `mkdir()` failure → -errno
- Directory operations → -ENOENT if not found

## 6. Testing Strategy

The automated test suite (`test_unionfs.sh`) validates:

1. **Layer Visibility**: Base files visible through mount
2. **Copy-on-Write**: Modifications don't affect base
3. **Whiteout Mechanism**: Deletions hide base files
4. **File Creation**: New files created in upper layer
5. **Directory Operations**: Mkdir/rmdir work correctly
6. **Directory Listing**: Merged view shows all files

Each test is independent and cleans up after itself.

## 7. Limitations and Future Improvements

1. **No Hard Links**: Cross-layer links not supported
2. **No Symbolic Links**: Not yet implemented
3. **No Metadata CoW**: Permissions/ownership not updated on writes
4. **No Atomic CoW**: Copy failures can leave inconsistent state
5. **No Sparse File Optimization**: Full file copied even for small writes
6. **No Multi-Layer Support**: Only two layers (lower/upper) supported

### Recommended Enhancements

- Implement lazy CoW (block-level, not whole-file)
- Add support for multiple lower layers
- Implement symbolic link handling
- Add fine-grained access control
- Optimize with memory-mapped I/O
- Implement transactional semantics for reliability

## 8. Building and Running

### Compilation
```bash
make clean && make
```

### Mounting
```bash
./mini_unionfs /path/to/lower /path/to/upper /mount/point
```

### Testing
```bash
make test
```

### Unmounting
```bash
fusermount -u /mount/point
# or
umount /mount/point
```

---

**Document Version**: 1.0  
**Date**: 2026-03-12  
**Project**: Mini-UnionFS (Cloud Computing Assignment)
