# Mini-UnionFS

A lightweight **union filesystem** built on top of FUSE (Filesystem in Userspace).  
It merges two directories — a read-only **lower layer** and a read-write **upper layer** — 
into a single unified mount point, using **Copy-on-Write (CoW)** semantics.

---

## Team Split

| Member   | Branch                          | Responsibility                      |
|----------|---------------------------------|-------------------------------------|
| Preksha  | `feature/preksha-fuse-setup`    | Core FUSE setup & mounting          |
| Member 2 | `feature/...`                   | (to be defined)                     |
| Member 3 | `feature/...`                   | (to be defined)                     |
| Member 4 | `feature/...`                   | (to be defined)                     |

---

## How It Works

```
lower_dir/          upper_dir/          mnt/  (unified view)
  hello.txt   +      (empty)      →       hello.txt  ← from lower
  data.txt            newfile.txt          data.txt   ← from lower
                                           newfile.txt← from upper
```

- **Reads** check `upper/` first, then fall back to `lower/`.
- **Writes** go to `upper/`. If the file only exists in `lower/`, it is first
  *copied up* to `upper/` (Copy-on-Write), then modified there — `lower/` is
  never touched.

---

## Project Structure

```
UnionFS/
├── src/
│   └── mini_unionfs.c      # Main FUSE implementation
├── include/
│   └── mini_unionfs.h      # Struct definitions & shared declarations
├── build/                  # Compiled object files (git-ignored)
├── docs/                   # Documentation
├── tests/                  # Test scripts (future)
├── Makefile
└── README.md
```

---

## Requirements (Ubuntu/Debian)

```bash
make install-deps
# installs: libfuse-dev  pkg-config  build-essential
```

---

## Build

```bash
make
```

The binary `./mini_unionfs` will be created in the project root.

---

## Quick Start

```bash
# 1. Create test directories with sample files
make setup-test

# 2. Mount (foreground — you will see FUSE log output, Ctrl-C to stop)
make mount

# 3. In another terminal, explore the merged view
ls mnt/          # shows files from both lower_dir/ and upper_dir/
cat mnt/hello.txt

# Write a new file — it lands in upper_dir/, lower_dir/ is untouched
echo "new data" > mnt/newfile.txt
cat upper_dir/newfile.txt   # visible here

# Modify a lower-layer file — CoW kicks in
echo "modified" > mnt/hello.txt
cat upper_dir/hello.txt     # copy now in upper
cat lower_dir/hello.txt     # original unchanged

# 4. Unmount
make unmount
```

---

## Manual Mount Command

```bash
./mini_unionfs <lower_dir> <upper_dir> <mount_point> [FUSE options]

# Examples:
./mini_unionfs ./lower ./upper ./mnt          # background
./mini_unionfs ./lower ./upper ./mnt -f       # foreground (debug-friendly)
./mini_unionfs ./lower ./upper ./mnt -f -d    # foreground + verbose FUSE debug
```

Unmount:
```bash
fusermount -u ./mnt
```

---

## FUSE Operations Implemented

| Operation   | Description                                        |
|-------------|----------------------------------------------------|
| `getattr`   | stat — upper first, fallback to lower              |
| `readdir`   | list directory merging both layers (no duplicates) |
| `open`      | open file — CoW triggered for write access         |
| `create`    | create new file in upper layer                     |
| `read`      | pread from open file descriptor                    |
| `write`     | pwrite to open file descriptor (upper only)        |
| `release`   | close file descriptor                              |
| `truncate`  | resize file — CoW if needed                        |
| `mkdir`     | create directory in upper layer                    |
| `rmdir`     | remove directory from upper layer                  |
| `unlink`    | delete file from upper layer                       |
| `rename`    | rename/move within upper layer                     |
| `chmod`     | change permissions — CoW if needed                 |
| `chown`     | change ownership — CoW if needed                   |

---

## Known Limitations / Future Work

- **Whiteout files**: deleting a lower-layer file does not permanently hide it
  (it reappears after remount). Whiteout support is a planned enhancement.
- **Hard links**: not currently supported across layers.
- **xattr support**: not yet implemented.
- **Multi-lower layers**: currently only one lower + one upper. Extending to
  `N` lower layers is a future task.

---

## Merge Guide (for teammates)

```bash
# Switch back to main and merge Preksha's branch:
git checkout main
git merge --no-ff feature/preksha-fuse-setup -m "Merge Preksha: core FUSE setup"

# Merge your own feature branch:
git merge --no-ff feature/<your-branch>
```
