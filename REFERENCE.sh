#!/usr/bin/env bash
# QUICK REFERENCE - Copy and paste these commands

# =============================================================================
# YOUR CURRENT STATUS
# =============================================================================

# Current Branch
git status

# View your commits
git log --oneline -5

# See branch structure
git branch -v

# =============================================================================
# NEXT STEPS (IN ORDER)
# =============================================================================

# 1. UNDERSTAND YOUR WORK
cat docs/QUICK_START_NAVYASHREE.md

# 2. TEST COMPILATION (optional at this point)
# (Your modules aren't integrated yet, so don't worry about linker errors)
make clean && make

# 3. INTEGRATE INTO MAIN DRIVER
# Edit src/mini_unionfs.c:
#   - Add includes at top: #include "path_resolution.h"
#   - Remove duplicate functions
#   - Update Makefile SOURCES variable

# 4. COMPILE AGAIN
make clean && make

# 5. RUN TESTS
make test

# 6. PUSH YOUR WORK
git push origin feature/path-resolution-layer-logic

# 7. CREATE PULL REQUEST
# Go to GitHub/GitLab and create PR to merge into 'main'

# =============================================================================
# KEY FILES FOR YOUR REFERENCE
# =============================================================================

# Start here:
#   📄 docs/BRANCH_SUMMARY.md          - Complete overview of your branch
#   📄 docs/QUICK_START_NAVYASHREE.md  - Step-by-step integration guide

# Code you wrote:
#   📝 src/path_resolution.h/.c        - Path resolution module
#   📝 src/directory_merge.h/.c        - Merged directory logic

# Team coordination:
#   📋 docs/TEAM_STRUCTURE.md          - Who does what
#   📋 README.md                       - Git workflow guide
#   📋 docs/DESIGN.md                  - System architecture

# =============================================================================
# GIT WORKFLOW REMINDER
# =============================================================================

# You are on:
# feature/path-resolution-layer-logic

# To sync with main later:
# git fetch origin
# git rebase origin/main

# To view all branches:
# git branch -a

# To see what changed:
# git diff main...feature/path-resolution-layer-logic

# =============════════════════════════════════════════════════════════════════
# SUCCESS CRITERIA
# ════════════════════════════════════════════════════════════════════════════

# Before creating PR, verify ALL:
# ✓ Code compiles: make clean && make
# ✓ Tests pass: make test  (6/6 tests)
# ✓ Makefile SOURCES updated
# ✓ No duplicate functions
# ✓ No errors or warnings

# =============================================================================
# QUICK HELP
# ════════════════════════════════════════════════════════════════════════════

# "How do I integrate my code?"
# → Read: docs/QUICK_START_NAVYASHREE.md

# "What functions did I implement?"
# → Read: src/path_resolution.h and src/directory_merge.h (comments explain)

# "What should happen when tests run?"
# → Read: docs/TASK_PATH_RESOLUTION.md (Testing section)

# "How do I create a PR?"
# → Read: docs/PR_TEMPLATE.md

# "What do other team members do?"
# → Read: docs/TEAM_STRUCTURE.md

# =============================================================================
# IMPORTANT REMINDERS
# ════════════════════════════════════════════════════════════════════════════

# 1. Your code is the FOUNDATION - other modules depend on it
# 2. All documentation is ready - no need to create more docs
# 3. The tests will tell you if integration is correct
# 4. Your functions are PUBLIC - others will use them
# 5. Git branch keeps your work isolated from others' work

# =============════════════════════════════════════════════════════════════════
