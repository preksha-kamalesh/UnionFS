# Mini-UnionFS - Team Collaboration Guide

## Current Project Status

**Main Branch:** `main` - Production-ready baseline  
**Total Team Members:** 4  
**Active Development Branches:**
- `feature/path-resolution-layer-logic` (Navyashree) - Path resolution & merged directory logic
- (Create your own branches for your work)

---

## For Each Team Member

### Your Task Assignment
1. **Navyashree**: Path Resolution & Layer Logic (`feature/path-resolution-layer-logic`)
2. **Team Member 2**: File I/O Operations (reads/writes)
3. **Team Member 3**: Copy-on-Write Implementation
4. **Team Member 4**: Deletion & Whiteout Operations

### Creating Your Branch
```bash
# Clone the repository (if you haven't already)
git clone <repo-url>
cd UnionFS

# Create your feature branch from main
git checkout main
git pull
git checkout -b feature/your-feature-name

# Example:
# git checkout -b feature/copy-on-write
# git checkout -b feature/file-io-operations
```

### Common Git Workflow

```bash
# 1. Make sure you're on your branch
git status

# 2. Make changes to your files
# ... edit src/your_module.c ...

# 3. Check what changed
git diff

# 4. Stage your changes
git add src/your_module.c
# or add everything
git add -A

# 5. Commit with descriptive message
git commit -m "feat: implement path resolution for whiteout files"
# Format: [type]: [description]
# Types: feat, fix, refactor, docs, test, chore

# 6. Push to repository
git push origin feature/your-feature-name

# 7. Create Pull Request (on GitHub/GitLab)
# Link to main branch for review
```

### Commit Message Format

Use conventional commits:
```
feat: add feature description
fix: fix description
refactor: refactor description
docs: update documentation
test: add/update tests
chore: maintenance tasks
```

Example:
```
git commit -m "feat: implement resolve_path() with whiteout checking"
git commit -m "fix: correct path construction for root directory files"
git commit -m "docs: add path resolution algorithm explanation"
```

---

## Working with Multiple Branches

### Viewing All Branches
```bash
git branch -a
git branch -v        # with last commit info
```

### Switching Between Branches
```bash
git checkout feature/path-resolution-layer-logic
git checkout feature/copy-on-write
git checkout main
```

### Updating Your Branch with Latest Main
```bash
git fetch origin
git rebase origin/main
# or merge if you prefer:
git merge origin/main
```

### Seeing Branch History
```bash
git log --oneline --graph --all
```

---

## Code Integration Strategy

### Before Starting on Someone's Code

Check **what's already implemented** in each module:

- [x] **path_resolution.c** (Navyashree) - CORE FOUNDATION
  - resolve_path()
  - construct_path()
  - get_whiteout_name()
  - is_whiteouted()

- [ ] **file_io.c** (Team Member 2) - READS/WRITES
  - unionfs_read()
  - unionfs_write()

- [ ] **copy_on_write.c** (Team Member 3) - CoW LOGIC
  - unionfs_open()
  - copy_file_to_upper()

- [ ] **deletion.c** (Team Member 4) - DELETION HANDLING
  - unionfs_unlink()
  - unionfs_rmdir()

### Dependency Graph

```
path_resolution.c (Foundation - started first)
        ↓
        ├→ directory_merge.c (uses path_resolution)
        ├→ file_io.c (uses path_resolution)
        ├→ copy_on_write.c (uses path_resolution)
        └→ deletion.c (uses path_resolution)
```

**Navyashree's module must be working first!**

---

## Testing Across Branches

### Branch-Specific Tests
```bash
# Test only your feature
make test

# Compile without testing
make clean && make
```

### Integration Testing (All Features)
```bash
# On main branch after all PRs merged
git checkout main
git pull
make clean && make
make test
# All 6 tests should pass
```

---

## Handling Merge Conflicts

If merging main into your branch causes conflicts:

### Option 1: Rebase (Cleaner History)
```bash
git fetch origin
git rebase origin/main

# If conflicts occur:
# 1. Edit conflicting files manually
# 2. Keep the needed changes
# 3. git add <file>
# 4. git rebase --continue
```

### Option 2: Merge (Three-way Merge)
```bash
git fetch origin
git merge origin/main

# If conflicts occur:
# 1. Edit conflicting files
# 2. git add <file>
# 3. git commit -m "Merge main into feature branch"
```

---

## Pull Request Checklist

Before creating a PR:

- [ ] Code compiles without errors/warnings: `make clean && make`
- [ ] Your tests pass: `make test`
- [ ] Code is commented and documented
- [ ] No debugging print statements (or mark them clearly)
- [ ] Commits are clean and well-organized
- [ ] Branch is up to date with main: `git rebase origin/main`

---

## File Organization

```
UnionFS/
├── src/
│   ├── mini_unionfs.c          (Main FUSE driver - integrates all modules)
│   ├── path_resolution.h/.c    (Navyashree's module) ✅
│   ├── directory_merge.h/.c    (Navyashree's module) ✅
│   ├── file_io.h/.c            (Team Member 2's module)
│   ├── copy_on_write.h/.c      (Team Member 3's module)
│   └── deletion.h/.c           (Team Member 4's module)
├── tests/
│   └── test_unionfs.sh         (Automated test suite)
├── docs/
│   ├── DESIGN.md               (System design overview)
│   ├── TASK_PATH_RESOLUTION.md (Navyashree's detailed task)
│   └── (Add more docs as needed)
├── Makefile
└── README.md (this file)
```

---

## Updating mini_unionfs.c for Integration

The main driver file needs minimal changes to include new modules:

```c
#include "path_resolution.h"
#include "directory_merge.h"
#include "file_io.h"
#include "copy_on_write.h"
#include "deletion.h"

static struct fuse_operations unionfs_oper = {
    .getattr = unionfs_getattr,    // from directory_merge.c
    .readdir = unionfs_readdir,    // from directory_merge.c
    .read = unionfs_read,          // from file_io.c
    .write = unionfs_write,        // from file_io.c
    .open = unionfs_open,          // from copy_on_write.c
    .unlink = unionfs_unlink,      // from deletion.c
    .rmdir = unionfs_rmdir,        // from deletion.c
    // ... other operations ...
};
```

**Who updates this?** The team lead or whoever is integrating final code.

---

## Communication

### Before Starting Work
Post in team chat:
```
"Starting on [feature/branch-name]"
"Implementation plan: [brief description]"
```

### While Working
Update your progress:
```
"Trying to fix path construction issue"
"Has anyone tested whiteout mechanism?"
```

### When Complete
```
"[feature/name] complete and tested"
"Ready for code review"
"PR created: [link]"
```

---

## Useful Git Commands Reference

```bash
# Status and info
git status                    # See current state
git log --oneline            # See recent commits
git diff                      # See uncommitted changes
git diff --staged            # See staged changes

# Branching
git branch -a                # List all branches
git checkout -b new-branch   # Create and switch
git checkout branch-name     # Switch branch
git branch -d branch-name    # Delete branch (local)

# Syncing
git fetch origin             # Get latest without merging
git pull origin main         # Fetch and merge main
git push origin branch-name  # Push your branch

# Undoing
git restore file.c           # Discard changes in file
git reset HEAD file.c        # Unstage file
git revert <commit>          # Undo a commit

# Debugging
git log --oneline --all --graph   # Visual branch history
git show <commit>                  # See what changed in commit
```

---

## Success Criteria for Team Completion

- [x] All team members have feature branches
- [x] Code modules are implemented independently
- [ ] All automated tests pass (6/6)
- [ ] Code compiles without warnings
- [ ] Design document is complete
- [ ] All features merged to main
- [ ] Project ready for submission

---

## Questions?

Talk to your team lead or post in team chat. Don't let issues block you—reach out!

Happy coding! 🚀
