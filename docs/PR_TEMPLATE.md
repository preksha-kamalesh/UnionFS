# Pull Request Template
## For Mini-UnionFS Team Integration

Use this template when creating a PR to merge your feature branch back into `main`.

---

## Example: Path Resolution Feature PR

### Title
```
feat: implement path resolution and directory merging
```

### Description

**What does this PR do?**
```
This PR implements the core path resolution layer and merged directory logic
for Mini-UnionFS. It enables the filesystem to:
- Determine file locations across upper/lower layers
- Present a unified view of both directories to users
- Handle file precedence correctly (upper overrides lower)
- Filter out whiteout markers from user-visible listings
```

**Related Tasks**
- Navyashree's path resolution assignment
- Foundation for all other FUSE operations

**Key Changes**
- Add `path_resolution.{h,c}` - core layer precedence algorithm
- Add `directory_merge.{h,c}` - merged view logic
- Update documentation with detailed task guide

---

## Testing

**How was this tested?**
```bash
$ make clean && make
# Compiled successfully with no errors/warnings

$ make test
# All 6 automated tests passed:
# ✓ Test 1: Layer Visibility
# ✓ Test 2: Copy-on-Write
# ✓ Test 3: Whiteout mechanism
# ✓ Test 4: File Creation
# ✓ Test 5: Directory Listing
# ✓ Test 6: Directory Creation
```

**Manual Testing Done**
```bash
$ ./mini_unionfs /tmp/lower /tmp/upper /tmp/mnt
$ ls /tmp/mnt/          # Shows merged files correctly
$ cat /tmp/mnt/file.txt # Can read from lower
```

---

## Code Quality

- [x] Code compiles without errors or warnings
- [x] All functions are well-documented with comments
- [x] Function names are clear and descriptive
- [x] No debugging print statements left in code
- [x] Error handling is consistent
- [x] No memory leaks

---

## Checklist Before Merge

- [x] Feature is complete
- [x] All tests pass
- [x] Documentation updated
- [x] Code reviewed by self and team members
- [x] Branch is up to date with main
- [x] No merge conflicts
- [x] Integration with mini_unionfs.c complete

---

## Integration Notes

This PR is **foundational** - other features depend on it:
- File I/O operations use `resolve_path()`
- Copy-on-Write checks whiteouts
- Deletion handling creates whiteouts

Merge only when all tests pass.

---

## Reviewer Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Algorithm logic is correct
- [ ] Path handling is safe
- [ ] No edge cases missed
- [ ] Documentation is clear
- [ ] Ready to merge

---

**Branch:** `feature/path-resolution-layer-logic`  
**Mergeable into:** `main`  
**Status:** Ready for merge once integration is complete
