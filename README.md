# Mini-UnionFS: A FUSE-Based Union Filesystem

Mini-UnionFS is a lightweight implementation of a union filesystem for Linux using FUSE (Filesystem in Userspace). It demonstrates how an overlay filesystem can merge a read-only lower directory with a read-write upper directory, implementing copy-on-write semantics and whiteout-based deletion tracking.

## What it Does

Mini-UnionFS stacks two directories:
- **Lower directory** (read-only): The base layer, typically containing template data or defaults
- **Upper directory** (read-write): The modification layer, holding copies and new files

When users interact with the mounted filesystem, the C code handles layer precedence, transparent file copying, and deletion masking—all without modifying the lower layer directly.

## Build and Run

### Prerequisites
- FUSE library (libfuse-dev on Debian/Ubuntu)
- GCC compiler
- Linux kernel with FUSE support (or WSL2/Docker)

### Compilation
```bash
make clean && make
```

### Running the Filesystem
```bash
./mini_unionfs -l /path/to/lower -u /path/to/upper /path/to/mount
```

Example:
```bash
mkdir -p lower upper mnt
echo "base" > lower/base.txt
./mini_unionfs -l $(pwd)/lower -u $(pwd)/upper $(pwd)/mnt
```

To unmount:
```bash
fusermount -u ./mnt
```

### Test Suite
```bash
make test
```

Runs 6 automated tests covering layer visibility, copy-on-write, whiteout mechanism, file creation, directory listing, and directory operations.

## Architecture Overview

The implementation consists of three core components:

1. **mini_unionfs.c** - FUSE operation handlers and filesystem callbacks
2. **path_resolution.c** - Layer precedence resolver and whiteout checking
3. **path_resolution.h** - Header with type definitions and function declarations

Key design decisions:
- Whiteout markers use the .wh. prefix to hide lower files without deletion
- Copy-on-write is triggered on file open with write flags
- Directory listing merges entries from both layers, hiding duplicates and whiteouts
- Parent directory hierarchy is auto-created when writing to nested paths

## Live Demonstration: The Four Core Mechanisms

The project can be verified with four terminal-based checks. Each one shows one core filesystem behavior.

---

### Demonstration 1: The Merged View (Path Resolution and Directory Listing)

**Project behavior:** The filesystem gives priority to the upper layer and merges file lists from both layers without duplicates.

**Code sections:** resolve_path_with_dirs() and unionfs_readdir()

**Preparation:**
```bash
echo "lower content" > lower/base.txt
echo "upper content" > upper/upper.txt
echo "lower version" > lower/conflict.txt
echo "upper version" > upper/conflict.txt
```

**Terminal demonstration:**
```bash
ls mnt/
# Output shows all three files merged together

cat mnt/conflict.txt
# Output: upper version (proves priority)
```

**Result:** You see all files in one merged view. If a file exists in both layers, the upper file is used.

---

### Demonstration 2: Copy-on-Write Trigger

**Project behavior:** If a file exists only in lower and you write to it, the file is copied to upper first, then modified there.

**Code sections:** unionfs_open(), copy_up_if_needed(), and copy_file_to_upper()

**Preparation:**
```bash
echo "original data" > lower/data.txt
# Upper directory remains empty
```

**Terminal demonstration:**
```bash
echo "added text" >> mnt/data.txt
# This triggers the copy-on-write mechanism

cat lower/data.txt
# Output: original data (unchanged)

ls upper/
# Shows: data.txt (now exists in upper)

cat mnt/data.txt
# Output: original data + added text
```

**Result:** The lower file stays unchanged. The new version is created and updated in upper.

---

### Demonstration 3: Creating New Data and Nested Paths

**Project behavior:** New folders and files are created only in upper. Missing parent folders are created automatically.

**Code sections:** ensure_parent_dirs(), unionfs_mkdir(), and unionfs_create()

**Preparation:**
```bash
# Start with clean lower and upper directories
# Only mnt is available for writing
```

**Terminal demonstration:**
```bash
mkdir -p mnt/new_folder/sub_folder/

echo "hello world" > mnt/new_folder/sub_folder/new_doc.txt

ls -R lower/
# Output shows only original files, nothing new

ls -R upper/
# Output shows the complete nested structure:
# new_folder/
# new_folder/sub_folder/
# new_folder/sub_folder/new_doc.txt
```

**Result:** Nested paths are created correctly in upper, and lower remains unchanged.

---

### Demonstration 4: Whiteout Deletion Illusion

**Project behavior:** Deleting a lower-layer file from the mount creates a whiteout marker in upper, which hides the file.

**Code sections:** get_whiteout_name(), unionfs_unlink(), and is_whiteouted_internal()

**Preparation:**
```bash
echo "delete me" > lower/delete_this.txt
# Upper directory is empty
```

**Terminal demonstration:**
```bash
ls mnt/delete_this.txt
# File appears normally

rm mnt/delete_this.txt
# Delete the file

ls mnt/ | grep delete_this.txt
# Output is empty (file appears deleted)

cat lower/delete_this.txt
# Output: delete me (file still exists physically)

ls -a upper/
# Output shows: .wh.delete_this.txt (the whiteout marker)
```

**Result:** The file disappears in the mount view, but the real lower file still exists. A `.wh.*` file in upper stores the delete marker.

---

## Project Requirements Coverage

This implementation fulfills all course project requirements:

1. **Layer Stacking** - Demonstrated in mechanisms 1 and 4
2. **Copy-on-Write** - Demonstrated in mechanism 2
3. **Whiteout-Based Deletion** - Demonstrated in mechanism 4
4. **POSIX Operations** - Demonstrated in mechanisms 2 and 3

## File Structure

```
UnionFS/
├── src/
│   ├── mini_unionfs.c          Main FUSE operations
│   ├── path_resolution.c       Layer precedence resolver
│   └── path_resolution.h       Header definitions
├── tests/
│   └── test_unionfs.sh         Automated test suite
├── docs/
│   └── DESIGN.md               Detailed design documentation
├── Makefile                    Build configuration
├── README.md                   This file
└── .github/workflows/
    └── ci.yml                  Continuous integration pipeline
```

## Design Highlights

### Path Resolution Algorithm

The resolve_path_with_dirs() function implements core layer precedence:

1. Check if file is whiteouted in upper layer (if yes, return NOT_FOUND)
2. If file exists in upper layer, resolve from there
3. Otherwise, resolve from lower layer
4. If file is in neither, return NOT_FOUND

### Directory Merging Strategy

When reading directories, unionfs_readdir() prevents duplicates by:
- Reading entries from both layers into a combined set
- Filtering out entries that are whiteouted
- Returning deduplicated results to FUSE

### Copy-on-Write Activation

CoW is triggered in unionfs_open() when:
- File is opened with write flags (O_WRONLY or O_RDWR)
- File exists in lower layer but not in upper layer
- At which point copy_file_to_upper() transfers all bytes to upper

### Whiteout Declaration

Deletion in unionfs_unlink() works by:
- Creating a special .wh. prefix file in the upper directory
- is_whiteouted_internal() checks for this marker during resolution
- The lower file never gets deleted, just masked

## Testing

The test suite validates:
- Layer visibility (upper layer shadowing lower layer)
- Copy-on-write mechanics (file copies on write, lower unchanged)
- Whiteout masking (deleted files remain hidden)
- File creation (new files go to upper layer)
- Directory merging (no duplicates in listings)
- Directory operations (mkdir and rmdir work through layers)

Run tests with:
```bash
make test
```

All six tests passing indicates correct implementation of every core mechanism.

## Limitations

- FUSE performance overhead compared to kernel-space filesystems
- Threading Model assumes single-threaded FUSE context
- Symlinks and special files not fully supported
- No permission inheritance beyond basic mode bits
- Limited extended attributes support

## References

- FUSE 31 API documentation
- Linux Unionfs and Overlayfs design patterns
- Copy-on-write filesystem semantics

## Project Status

This Mini-UnionFS implementation is complete with all four mechanisms working correctly and validated through automated testing.

Build status: Passing
Test status: 6 out of 6 tests passing
Documentation status: Complete and aligned with implementation
