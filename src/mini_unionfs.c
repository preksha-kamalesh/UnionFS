/*
 * mini_unionfs.c — Mini-UnionFS: Core FUSE Implementation
 *
 * Author : Preksha (Core FUSE Setup & Mounting)
 * Branch : feature/preksha-fuse-setup
 *
 * Implements a two-layer union filesystem using FUSE:
 *   - lower_dir  : read-only base layer
 *   - upper_dir  : read-write layer  (Copy-on-Write destination)
 *   - mount_point: unified view presented to the user
 *
 * Build  : make
 * Mount  : ./mini_unionfs <lower_dir> <upper_dir> <mount_point> [fuse_opts]
 * Unmount: fusermount -u <mount_point>
 *
 * Tested on Ubuntu 22.04 with libfuse2 (FUSE 2.9.x).
 */

#define FUSE_USE_VERSION 26

#include "../include/mini_unionfs.h"

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

/* =========================================================================
 * Convenience macros
 * ====================================================================== */

/* Retrieve the global state from FUSE's private_data pointer */
#define UNIONFS_STATE \
    ((struct mini_unionfs_state *) fuse_get_context()->private_data)

/* Build an absolute path inside a layer directory.
 * e.g. layer="/upper", path="/foo/bar.txt"  →  buf="/upper/foo/bar.txt" */
#define BUILD_PATH(buf, layer, path) \
    snprintf((buf), sizeof(buf), "%s%s", (layer), (path))

/* =========================================================================
 * Utility helpers
 * ====================================================================== */

/*
 * make_dirs — recursive mkdir (equivalent to mkdir -p)
 *
 * Creates every component of <path>.  Safe against shell-injection because
 * it never calls system(); it uses the mkdir(2) syscall directly.
 */
int make_dirs(const char *path, mode_t mode)
{
    char   tmp[PATH_MAX];
    char  *p;
    size_t len;

    if (!path || *path == '\0') return -1;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    /* Strip trailing slash */
    if (len > 1 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    /* Walk through every '/' and create each prefix in turn */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) < 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }

    /* Create the final (deepest) component */
    if (mkdir(tmp, mode) < 0 && errno != EEXIST)
        return -1;

    return 0;
}

/*
 * unionfs_copy_up — Copy-on-Write: bring a file from lower into upper.
 *
 * Before the first write to a file that exists only in lower_dir, we copy
 * the file (preserving permissions) into upper_dir so writes go there.
 * Parent directories in upper are created as needed.
 *
 * Returns 0 on success, -1 on error.
 */
int unionfs_copy_up(const struct mini_unionfs_state *state, const char *path)
{
    char lower_path[PATH_MAX];
    char upper_path[PATH_MAX];
    char upper_parent[PATH_MAX];
    char buf[65536];
    struct stat st;
    ssize_t nr, nw;
    int src_fd = -1, dst_fd = -1, ret = -1;

    BUILD_PATH(lower_path, state->lower_dir, path);
    BUILD_PATH(upper_path, state->upper_dir, path);

    /* --- 1. Create parent directories in upper layer ------------------- */
    snprintf(upper_parent, sizeof(upper_parent), "%s", upper_path);
    if (make_dirs(dirname(upper_parent), 0755) < 0)
        return -1;

    /* --- 2. Stat the source file --------------------------------------- */
    if (lstat(lower_path, &st) < 0)
        return -1;

    /* --- 3. Open source (lower) ---------------------------------------- */
    src_fd = open(lower_path, O_RDONLY);
    if (src_fd < 0)
        goto out;

    /* --- 4. Create destination (upper) with same permissions ----------- */
    dst_fd = open(upper_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dst_fd < 0)
        goto out;

    /* --- 5. Stream copy ------------------------------------------------ */
    while ((nr = read(src_fd, buf, sizeof(buf))) > 0) {
        nw = write(dst_fd, buf, (size_t)nr);
        if (nw != nr) {
            goto out;
        }
    }
    if (nr < 0)
        goto out;

    ret = 0;   /* success */

out:
    if (src_fd >= 0) close(src_fd);
    if (dst_fd >= 0) close(dst_fd);
    return ret;
}

/* =========================================================================
 * FUSE operation implementations
 * ====================================================================== */

/* -------------------------------------------------------------------------
 * getattr — stat(2) equivalent
 *
 * Check upper layer first; fall back to lower.  This ensures that files
 * modified/created in upper shadow their lower counterparts.
 * ---------------------------------------------------------------------- */
static int unionfs_getattr(const char *path, struct stat *stbuf)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);
    BUILD_PATH(lower_path, state->lower_dir, path);

    if (lstat(upper_path, stbuf) == 0)
        return 0;

    if (lstat(lower_path, stbuf) == 0)
        return 0;

    return -ENOENT;
}

/* -------------------------------------------------------------------------
 * readdir — list directory contents merging both layers
 *
 * Files/dirs present in upper shadow the same names from lower.
 * ---------------------------------------------------------------------- */
static int unionfs_readdir(const char *path, void *buf,
                           fuse_fill_dir_t filler, off_t offset,
                           struct fuse_file_info *fi)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];
    char check_upper[PATH_MAX];
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    BUILD_PATH(upper_path, state->upper_dir, path);
    BUILD_PATH(lower_path, state->lower_dir, path);

    /* Always add "." and ".." */
    filler(buf, ".",  NULL, 0);
    filler(buf, "..", NULL, 0);

    /* --- Pass 1: upper layer ------------------------------------------ */
    dp = opendir(upper_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 ||
                strcmp(de->d_name, "..") == 0)
                continue;
            filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
    }

    /* --- Pass 2: lower layer (skip names already served from upper) ---- */
    dp = opendir(lower_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 ||
                strcmp(de->d_name, "..") == 0)
                continue;
            /* Only add if not already present in upper */
            snprintf(check_upper, sizeof(check_upper),
                     "%s/%s", upper_path, de->d_name);
            if (access(check_upper, F_OK) != 0)
                filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * open — open(2) equivalent
 *
 * Read-only opens try upper then lower.
 * Write opens trigger a copy-up if the file exists only in lower.
 * ---------------------------------------------------------------------- */
static int unionfs_open(const char *path, struct fuse_file_info *fi)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];
    int fd;

    BUILD_PATH(upper_path, state->upper_dir, path);
    BUILD_PATH(lower_path, state->lower_dir, path);

    /* --- Try opening from upper layer first ---------------------------- */
    fd = open(upper_path, fi->flags);
    if (fd >= 0) {
        fi->fh = (uint64_t)fd;
        return 0;
    }

    /* --- Write access: copy-up from lower if the file is there --------- */
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        if (access(lower_path, F_OK) == 0) {
            if (unionfs_copy_up(state, path) != 0)
                return -EIO;
            fd = open(upper_path, fi->flags);
            if (fd >= 0) {
                fi->fh = (uint64_t)fd;
                return 0;
            }
        }
        return -errno;
    }

    /* --- Read-only: fall back to lower layer --------------------------- */
    fd = open(lower_path, fi->flags);
    if (fd >= 0) {
        fi->fh = (uint64_t)fd;
        return 0;
    }

    return -ENOENT;
}

/* -------------------------------------------------------------------------
 * create — create a new file (always in upper layer)
 * ---------------------------------------------------------------------- */
static int unionfs_create(const char *path, mode_t mode,
                          struct fuse_file_info *fi)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];
    char parent_dir[PATH_MAX];
    int fd;

    BUILD_PATH(upper_path, state->upper_dir, path);

    /* Ensure parent directories exist in upper */
    snprintf(parent_dir, sizeof(parent_dir), "%s", upper_path);
    if (make_dirs(dirname(parent_dir), 0755) < 0)
        return -errno;

    fd = open(upper_path, fi->flags | O_CREAT | O_TRUNC, mode);
    if (fd < 0)
        return -errno;

    fi->fh = (uint64_t)fd;
    return 0;
}

/* -------------------------------------------------------------------------
 * read — pread from the file descriptor stored in fi->fh
 * ---------------------------------------------------------------------- */
static int unionfs_read(const char *path, char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi)
{
    int res;
    (void) path;

    res = (int)pread((int)fi->fh, buf, size, offset);
    if (res < 0)
        return -errno;
    return res;
}

/* -------------------------------------------------------------------------
 * write — pwrite to the file descriptor stored in fi->fh
 *         The file is always in upper at this point (open handled CoW).
 * ---------------------------------------------------------------------- */
static int unionfs_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi)
{
    int res;
    (void) path;

    res = (int)pwrite((int)fi->fh, buf, size, offset);
    if (res < 0)
        return -errno;
    return res;
}

/* -------------------------------------------------------------------------
 * release — close the file descriptor when the last reference is dropped
 * ---------------------------------------------------------------------- */
static int unionfs_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;
    close((int)fi->fh);
    return 0;
}

/* -------------------------------------------------------------------------
 * truncate — resize a file (copy-up first if file is in lower only)
 * ---------------------------------------------------------------------- */
static int unionfs_truncate(const char *path, off_t size)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (access(upper_path, F_OK) != 0) {
        if (unionfs_copy_up(state, path) != 0)
            return -EIO;
    }

    if (truncate(upper_path, size) < 0)
        return -errno;
    return 0;
}

/* -------------------------------------------------------------------------
 * mkdir — create a directory in the upper layer
 * ---------------------------------------------------------------------- */
static int unionfs_mkdir(const char *path, mode_t mode)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (mkdir(upper_path, mode) < 0)
        return -errno;
    return 0;
}

/* -------------------------------------------------------------------------
 * rmdir — remove a directory from the upper layer
 * ---------------------------------------------------------------------- */
static int unionfs_rmdir(const char *path)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (rmdir(upper_path) < 0)
        return -errno;
    return 0;
}

/* -------------------------------------------------------------------------
 * unlink — remove a file from the upper layer
 *
 * Note: if the file exists only in lower, the delete will appear to succeed
 * but the file will reappear on remount (whiteout support is a future task).
 * ---------------------------------------------------------------------- */
static int unionfs_unlink(const char *path)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (unlink(upper_path) < 0)
        return -errno;
    return 0;
}

/* -------------------------------------------------------------------------
 * rename — rename/move within the upper layer
 * ---------------------------------------------------------------------- */
static int unionfs_rename(const char *from, const char *to)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_from[PATH_MAX];
    char upper_to[PATH_MAX];

    BUILD_PATH(upper_from, state->upper_dir, from);
    BUILD_PATH(upper_to,   state->upper_dir, to);

    if (rename(upper_from, upper_to) < 0)
        return -errno;
    return 0;
}

/* -------------------------------------------------------------------------
 * chmod / chown — applied to the upper layer copy (copy-up if needed)
 * ---------------------------------------------------------------------- */
static int unionfs_chmod(const char *path, mode_t mode)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (access(upper_path, F_OK) != 0) {
        if (unionfs_copy_up(state, path) != 0)
            return -EIO;
    }

    if (chmod(upper_path, mode) < 0)
        return -errno;
    return 0;
}

static int unionfs_chown(const char *path, uid_t uid, gid_t gid)
{
    struct mini_unionfs_state *state = UNIONFS_STATE;
    char upper_path[PATH_MAX];

    BUILD_PATH(upper_path, state->upper_dir, path);

    if (access(upper_path, F_OK) != 0) {
        if (unionfs_copy_up(state, path) != 0)
            return -EIO;
    }

    if (lchown(upper_path, uid, gid) < 0)
        return -errno;
    return 0;
}

/* =========================================================================
 * FUSE operations table — maps FUSE hooks to our implementations
 * ====================================================================== */
static struct fuse_operations unionfs_oper = {
    .getattr  = unionfs_getattr,
    .readdir  = unionfs_readdir,
    .open     = unionfs_open,
    .create   = unionfs_create,
    .read     = unionfs_read,
    .write    = unionfs_write,
    .release  = unionfs_release,
    .truncate = unionfs_truncate,
    .mkdir    = unionfs_mkdir,
    .rmdir    = unionfs_rmdir,
    .unlink   = unionfs_unlink,
    .rename   = unionfs_rename,
    .chmod    = unionfs_chmod,
    .chown    = unionfs_chown,
};

/* =========================================================================
 * Entrypoint & mount logic
 * ====================================================================== */

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "\nUsage:\n"
        "  %s <lower_dir> <upper_dir> <mount_point> [FUSE options]\n\n"
        "Arguments:\n"
        "  lower_dir    Read-only base layer  (source files)\n"
        "  upper_dir    Read-write top layer  (modifications land here)\n"
        "  mount_point  Where the merged filesystem appears\n\n"
        "FUSE options (examples):\n"
        "  -f           Run in foreground (useful for debugging)\n"
        "  -d           Enable FUSE debug output\n"
        "  -s           Single-threaded mode\n\n"
        "Examples:\n"
        "  # Foreground mount (Ctrl-C to unmount):\n"
        "  %s ./lower ./upper ./mnt -f\n\n"
        "  # Background mount:\n"
        "  %s ./lower ./upper ./mnt\n"
        "  fusermount -u ./mnt   # to unmount\n\n",
        prog, prog, prog);
}

int main(int argc, char *argv[])
{
    struct mini_unionfs_state *state;
    char *fuse_argv[argc];   /* at most as many args as given */
    int   fuse_argc;
    int   i;

    /* ------------------------------------------------------------------ */
    /* Argument validation                                                  */
    /* ------------------------------------------------------------------ */
    if (argc < 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------------ */
    /* Allocate and populate global state                                  */
    /* ------------------------------------------------------------------ */
    state = calloc(1, sizeof(struct mini_unionfs_state));
    if (!state) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    /* Resolve to absolute paths so FUSE cd-ing doesn't break us */
    state->lower_dir = realpath(argv[1], NULL);
    state->upper_dir = realpath(argv[2], NULL);

    if (!state->lower_dir) {
        fprintf(stderr, "Error: lower_dir '%s' does not exist or is not accessible.\n", argv[1]);
        free(state);
        return EXIT_FAILURE;
    }

    /* upper_dir will be created by realpath only if it already exists;
     * if not, create it and resolve again.                               */
    if (!state->upper_dir) {
        if (mkdir(argv[2], 0755) < 0 && errno != EEXIST) {
            fprintf(stderr, "Error: cannot create upper_dir '%s': %s\n",
                    argv[2], strerror(errno));
            free(state->lower_dir);
            free(state);
            return EXIT_FAILURE;
        }
        state->upper_dir = realpath(argv[2], NULL);
    }

    if (!state->upper_dir) {
        fprintf(stderr, "Error: upper_dir '%s' could not be resolved.\n", argv[2]);
        free(state->lower_dir);
        free(state);
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------------ */
    /* Print startup banner                                                 */
    /* ------------------------------------------------------------------ */
    printf("\n=== Mini-UnionFS ===\n");
    printf("  Lower (read-only) : %s\n", state->lower_dir);
    printf("  Upper (read-write): %s\n", state->upper_dir);
    printf("  Mount point       : %s\n", argv[3]);
    printf("====================\n\n");

    /* ------------------------------------------------------------------ */
    /* Build argv for fuse_main:                                           */
    /*   We received: prog lower upper mountpoint [fuse_opts...]           */
    /*   fuse_main expects: prog mountpoint [fuse_opts...]                 */
    /* ------------------------------------------------------------------ */
    fuse_argv[0] = argv[0];          /* program name  */
    fuse_argv[1] = argv[3];          /* mount point   */
    fuse_argc = 2;
    for (i = 4; i < argc; i++)       /* extra FUSE options */
        fuse_argv[fuse_argc++] = argv[i];

    /* ------------------------------------------------------------------ */
    /* Hand off to FUSE — this blocks until the filesystem is unmounted    */
    /* ------------------------------------------------------------------ */
    return fuse_main(fuse_argc, fuse_argv, &unionfs_oper, state);
}
