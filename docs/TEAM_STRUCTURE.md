# Team Project Structure & Division of Labor
## Mini-UnionFS - 4 Person Cloud Computing Project

---

## 🎯 Overall Goal

Build a **Mini-UnionFS** FUSE filesystem that merges layered directories (Docker-style).

**Success Criteria:**
- Implement Copy-on-Write semantics
- Handle whiteout (deletion) mechanism
- Support merged directory view
- Pass all 6 automated tests
- Complete design document
- Push to university Git repository

---

## 👥 Team Member Assignments

### 1️⃣ **Navyashree** - Path Resolution & Layer Logic ✅
**Branch:** `feature/path-resolution-layer-logic`

**What You're Building:**
- Path resolution engine (core of the system)
- Merged directory view logic
- File attribute handling

**Key Functions:**
- `resolve_path()` - determines where files are
- `unionfs_getattr()` - gets file attributes
- `unionfs_readdir()` - lists merged directories

**Status:** ✅ IMPLEMENTED (in your branch)

**Next Steps:**
1. Integrate code into main mini_unionfs.c
2. Update Makefile with new compilation sources
3. Run automated tests
4. Create Pull Request to main

**Deliverables:**
- ✅ path_resolution.h/c (complete)
- ✅ directory_merge.h/c (complete)
- ✅ Detailed documentation (complete)

---

### 2️⃣ **Team Member 2** - File I/O Operations
**Branch:** `feature/file-io-operations` (create this)

**What You're Building:**
- Read operations
- Write operations
- File opening/closing

**Key Functions:**
- `unionfs_read()` - read file contents
- `unionfs_write()` - write to files
- `unionfs_open()` - prepare file for I/O

**Dependencies:** Uses Navyashree's `resolve_path()`

**Tasks:**
```bash
# 1. Create your branch
git checkout main
git checkout -b feature/file-io-operations

# 2. Create src/file_io.h and src/file_io.c
# 3. Implement the three functions above
# 4. Test with manual I/O operations
# 5. Create Pull Request
```

---

### 3️⃣ **Team Member 3** - Copy-on-Write Implementation
**Branch:** `feature/copy-on-write` (create this)

**What You're Building:**
- Copy-on-Write logic
- File copying to upper layer
- Write permission handling

**Key Functions:**
- `unionfs_open()` - detect write operations on lower files
- `copy_file_to_upper()` - copy file from lower to upper
- Handle O_WRONLY, O_RDWR, O_APPEND flags

**Dependencies:** Uses Navyashree's `is_whiteouted()` and `resolve_path()`

**Example Logic:**
```c
// When user opens a lower_dir file with write flags:
if (file_in_lower && !file_in_upper && write_flags) {
    copy_file_from_lower_to_upper(file);  // CoW
}
```

---

### 4️⃣ **Team Member 4** - Deletion & Whiteout Operations
**Branch:** `feature/deletion-whiteout` (create this)

**What You're Building:**
- File deletion handling
- Directory deletion handling
- Whiteout file creation

**Key Functions:**
- `unionfs_unlink()` - delete files
- `unionfs_rmdir()` - delete directories
- Handle whiteout creation for lower layer files

**Dependencies:** Uses Navyashree's `get_whiteout_name()` and `is_whiteouted()`

**Example Logic:**
```c
// When user deletes a file:
if (file_in_upper) {
    unlink(upper/file);  // Physical delete
}
if (file_in_lower) {
    create(upper/.wh.file);  // Whiteout marker
}
```

---

## 📊 Dependency Graph

```
Path Resolution (Navyashree) ← FOUNDATION
        ↓
        ├→ Directory Merge (Navyashree)
        │   ↓ Uses: resolve_path()
        │
        ├→ File I/O (Team Member 2)
        │   ↓ Uses: resolve_path()
        │
        ├→ Copy-on-Write (Team Member 3)
        │   ↓ Uses: resolve_path(), is_whiteouted()
        │
        └→ Deletion (Team Member 4)
            ↓ Uses: get_whiteout_name(), is_whiteouted()
```

**Critical:** Navyashree's work must be integrated first!

---

## 📅 Recommended Timeline

### Week 1
- [x] Navyashree: Path resolution implementation ✅
- [ ] Other members: Create branches, start implementation
- [ ] Integrate Navyashree's work into main

### Week 2
- [ ] Team Member 2: File I/O implementation
- [ ] Team Member 3: Copy-on-Write implementation
- [ ] Team Member 4: Deletion implementation
- [ ] Ongoing: Testing and bug fixes

### Week 3
- [ ] All modules: Pull request reviews
- [ ] Merge all features to main
- [ ] Final integration testing (all 6 tests pass)
- [ ] Design document completion
- [ ] Git repository push

---

## 🔄 Git Workflow for Team

### For Each Team Member (except first commit)

```bash
# 1. START WORK
git checkout main
git pull origin main                    # Get latest
git checkout -b feature/your-feature    # Create your branch

# 2. MAKE CHANGES
# ... edit src/your_module.c ...
# ... edit tests, docs ...

# 3. TEST LOCALLY
make clean && make                      # Compile
make test                               # Run tests

# 4. COMMIT WORK
git add -A
git commit -m "feat: implement your feature"

# 5. BEFORE MERGE
git fetch origin
git rebase origin/main                  # Update with latest main

# 6. PUSH AND CREATE PR
git push origin feature/your-feature
# Create Pull Request on GitHub/GitLab linking to main

# 7. AFTER REVIEW
git checkout main
git pull origin main                    # Get merged version
```

---

## 📋 Integration Checklist

### Before Each Merge to Main

- [ ] Code compiles with no errors/warnings
- [ ] All tests pass (`make test` shows 6/6)
- [ ] No memory leaks
- [ ] Functions well-documented
- [ ] No debug print statements
- [ ] Branch up to date with main

### PR Description Template

```
## [Feature Name] Implementation

**What:** [Brief description]

**How:** [Key implementation details]

**Testing:** 
```bash
$ make test    # All 6 tests pass
```

**Dependencies:** Uses functions from [other modules]
```

---

## 🧪 Testing Strategy

### Individual Testing (Each Member)
```bash
# Test your module in isolation
./mini_unionfs /tmp/lower /tmp/upper /tmp/mnt
# Manual testing for your feature
```

### Integration Testing (Full Team)
```bash
# On main branch after all PRs merged
make clean && make
make test
# Expected: 6/6 tests pass
```

### Test Suite Components
1. **Layer Visibility** - Can see lower files through mount
2. **Copy-on-Write** - File modifications don't affect lower
3. **Whiteout Mechanism** - Deletions hide base files
4. **File Creation** - New files created in upper
5. **Directory Listing** - Merged view works correctly
6. **Directory Operations** - mkdir/rmdir work

---

## 📁 Final File Structure (After All Work)

```
UnionFS/
├── src/
│   ├── mini_unionfs.c              (Main driver - integrates all)
│   ├── mini_unionfs.h              (Declarations)
│   ├── path_resolution.h/.c        (Navyashree) ✅
│   ├── directory_merge.h/.c        (Navyashree) ✅
│   ├── file_io.h/.c                (Team Member 2) 
│   ├── copy_on_write.h/.c          (Team Member 3)
│   └── deletion.h/.c               (Team Member 4)
├── tests/
│   └── test_unionfs.sh             (Automated suite)
├── docs/
│   ├── DESIGN.md                   (System design)
│   ├── TASK_PATH_RESOLUTION.md     (Navyashree's task)
│   ├── QUICK_START_NAVYASHREE.md   (Getting started)
│   ├── PR_TEMPLATE.md              (PR guidelines)
│   └── (Member-specific docs)
├── Makefile
├── README.md
└── .git/                           (Git repository)
```

---

## 🚀 Key Milestones

- ✅ **Milestone 1:** Navyashree - Path resolution working
  - Status: COMPLETE
  - Deliverable: feature/path-resolution-layer-logic branch

- ⏳ **Milestone 2:** All members - Feature branches created
  - Deadline: End of Week 1
  - Checklist: Each member has feature branch

- ⏳ **Milestone 3:** All members - Implementation complete
  - Deadline: End of Week 2
  - Checklist: All features working in isolation

- ⏳ **Milestone 4:** Team - All features integrated
  - Deadline: End of Week 3
  - Checklist: 6/6 tests pass on main

- ⏳ **Milestone 5:** Team - Final submission
  - Deadline: Submission deadline
  - Deliverables: Code pushed + Design document

---

## 📞 Communication Protocol

### Daily Standup (Team Meeting)
```
Each member reports:
1. What I completed yesterday
2. What I'm working on today
3. Any blockers/challenges
```

### When Stuck
```
1. Try to solve for 15 minutes
2. Post in team chat with:
   - What you're trying to do
   - What error you're seeing
   - What you've already tried
3. Team helps or escalates to tech lead
```

### Before Merging
```
1. Let team know PR is ready: "PR ready for review: [link]"
2. Wait for at least 1 other member to approve
3. If all checks pass, merge to main
4. Announce merge: "Merged [feature] to main"
```

---

## ✨ Success Criteria (Final)

By submission deadline, the project must have:

- ✅ All source code in Git repository
- ✅ Working Makefile that compiles all modules
- ✅ All 6 automated tests passing
- ✅ 2-3 page design document (DESIGN.md)
- ✅ Clean, well-commented code
- ✅ Clear separation of labor in commits
- ✅ Evidence of team collaboration (multiple branches, PRs)

---

## 📚 Resources

Each team member has:
- ✅ Detailed task documentation
- ✅ Code templates and examples
- ✅ Automated test suite
- ✅ Team collaboration guide
- ✅ PR template
- ✅ Git workflow documentation

---

## 🎓 Learning Outcomes

By completing this project, you will understand:
- How containerization filesystems work (Docker foundation)
- FUSE (Filesystem in Userspace) programming
- Layered filesystem semantics
- Copy-on-Write mechanisms
- Version control and team collaboration
- System-level file I/O operations

---

**Project:** Mini-UnionFS  
**Team Size:** 4 members  
**Duration:** 3 weeks  
**Status:** Implementation in progress  
**Current Phase:** Navyashree's path resolution complete, ready for other modules
