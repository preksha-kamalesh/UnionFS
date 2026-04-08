/*
 * mini_unionfs.h — Mini-UnionFS: Header Definitions
 *
 * Author : Preksha (Core FUSE Setup & Mounting)
 * Branch : feature/preksha-fuse-setup
 *
 * Defines the global state structure and shared helpers used throughout
 * the Mini-UnionFS FUSE filesystem.
 *
 * Layer model:
 *   lower_dir  — read-only base layer  (e.g. a system image)
 *   upper_dir  — read-write top  layer  (all modifications land here)
 *   mount_point— where both layers appear merged to the user
 *
 * On a read  : upper is checked first; falls back to lower.
 * On a write : file is "copied up" from lower → upper if needed (CoW),
 *              then written to upper.
 */

#ifndef MINI_UNIONFS_H
#define MINI_UNIONFS_H

/* Declare the FUSE API version we target (FUSE 2.6+) before including fuse.h */
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <limits.h>
#include <sys/stat.h>

/* -------------------------------------------------------------------------
 * Global filesystem state
 * Stored as FUSE private_data; retrieved anywhere with:
 *   UNIONFS_STATE  (macro defined in mini_unionfs.c)
 * ---------------------------------------------------------------------- */
struct mini_unionfs_state {
    char *lower_dir;   /* Absolute path — read-only base layer            */
    char *upper_dir;   /* Absolute path — read-write layer (CoW target)   */
};

/* -------------------------------------------------------------------------
 * Copy-on-Write helper
 *
 * Copies the file at <path> from the lower layer into the upper layer,
 * creating any necessary parent directories.  Called automatically before
 * the first write to a file that exists only in lower.
 *
 * Returns  0 on success, -1 on error.
 * ---------------------------------------------------------------------- */
int unionfs_copy_up(const struct mini_unionfs_state *state, const char *path);

/* -------------------------------------------------------------------------
 * Directory helper
 *
 * Recursively creates all components of <path> (like mkdir -p).
 * Returns  0 on success, -1 on error (errno is set).
 * ---------------------------------------------------------------------- */
int make_dirs(const char *path, mode_t mode);

#endif /* MINI_UNIONFS_H */
