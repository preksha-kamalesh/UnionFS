#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#include "path_resolution.h"

#define MAX_PATH_LEN 4096
#define WHITEOUT_PREFIX ".wh."

/* Global state passed to FUSE */
#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

/* FUSE Operations */

/**
 * Get file attributes - handles path resolution and precedence
 *
 * This function implements the core "merge" logic for attributes:
 * - If file is whiteouted: return ENOENT (not found)
 * - If file in upper: return upper file's attributes
 * - If file in lower: return lower file's attributes
 * - If file nowhere: return ENOENT
 *
 * Returns the attributes of whichever version the user should see.
 */
static int unionfs_getattr(const char *path, struct stat *stbuf) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char resolved_path[MAX_PATH_LEN];
    int ret;
    
    /* Resolve path to actual location */
    ret = resolve_path_with_dirs(path, data->lower_dir, data->upper_dir, resolved_path);
    
    /* Handle different resolution outcomes */
    switch (ret) {
        case RESOLVE_WHITEOUTED:
            /* File is deleted (whiteouted) */
            return -ENOENT;
            
        case RESOLVE_NOT_FOUND:
            /* File doesn't exist in any layer */
            return -ENOENT;
            
        case RESOLVE_IN_UPPER:
        case RESOLVE_IN_LOWER:
            /* File found - get its attributes */
            if (lstat(resolved_path, stbuf) == 0) {
                return 0;
            } else {
                return -errno;
            }
            
        default:
            /* Should not reach here */
            return -EIO;
    }
}

/**
 * Read directory - merges listings from both layers
 *
 * Core algorithm:
 * 1. Track file names we've already added
 * 2. List all files from upper directory first (take precedence)
 *    - Skip whiteout files (they're internal markers)
 *    - Add to result
 * 3. List all files from lower directory
 *    - Skip if already in upper (avoid duplicates)
 *    - Skip if whiteouted (hidden by upper layer)
 *    - Add to result
 * 4. Free tracking structures
 *
 * This ensures user sees the unified view they expect.
 */
static int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi) {
    (void) offset; /* Unused */
    (void) fi;     /* Unused */
    
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    DIR *dp;
    struct dirent *de;
    
    /* Allocate tracking array for files seen in upper directory */
    char **upper_files = malloc(10000 * sizeof(char *));
    int upper_file_count = 0;
    
    if (!upper_files) {
        return -ENOMEM;
    }
    
    /* ========== PHASE 1: Read Upper Directory ========== */
    construct_path(data->upper_dir, path, upper_path);
    dp = opendir(upper_path);
    
    if (dp != NULL) {
        while ((de = readdir(dp)) != NULL) {
            /* Skip whiteout files - they're internal */
            if (strncmp(de->d_name, WHITEOUT_PREFIX, strlen(WHITEOUT_PREFIX)) == 0) {
                continue;
            }
            
            /* Add file to result */
            filler(buf, de->d_name, NULL, 0);
            
            /* Track that we've seen this file */
            upper_files[upper_file_count] = strdup(de->d_name);
            upper_file_count++;
            
            if (upper_file_count >= 9999) {
                break; /* Safety limit */
            }
        }
        closedir(dp);
    }
    
    /* ========== PHASE 2: Read Lower Directory ========== */
    construct_path(data->lower_dir, path, lower_path);
    dp = opendir(lower_path);
    
    if (dp != NULL) {
        while ((de = readdir(dp)) != NULL) {
            int is_duplicate = 0;
            int is_whiteouted_file = 0;
            
            /* Check if we've already listed this file from upper */
            for (int i = 0; i < upper_file_count; i++) {
                if (strcmp(upper_files[i], de->d_name) == 0) {
                    is_duplicate = 1;
                    break;
                }
            }
            
            if (is_duplicate) {
                /* File exists in both layers - upper takes precedence, skip */
                continue;
            }
            
            /* Check if this file is whiteouted */
            char full_path[MAX_PATH_LEN];
            if (strlen(path) == 1 && path[0] == '/') {
                /* File in root directory */
                snprintf(full_path, MAX_PATH_LEN, "/%s", de->d_name);
            } else {
                /* File in subdirectory */
                snprintf(full_path, MAX_PATH_LEN, "%s/%s", path, de->d_name);
            }
            
            if (is_whiteouted_internal(data->upper_dir, full_path)) {
                /* File is deleted by whiteout - hide it */
                is_whiteouted_file = 1;
            }
            
            if (!is_whiteouted_file) {
                /* Add file from lower layer to result */
                filler(buf, de->d_name, NULL, 0);
            }
        }
        closedir(dp);
    }
    
    /* ========== Cleanup ========== */
    for (int i = 0; i < upper_file_count; i++) {
        free(upper_files[i]);
    }
    free(upper_files);
    
    return 0;
}

static int unionfs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int res;
    
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char resolved_path[MAX_PATH_LEN];
    int ret = resolve_path_with_dirs(path, data->lower_dir, data->upper_dir, resolved_path);
    if (ret != RESOLVE_IN_UPPER && ret != RESOLVE_IN_LOWER) {
        return -ENOENT;
    }
    
    fd = open(resolved_path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }
    
    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }
    
    close(fd);
    return res;
}

static int unionfs_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int res;
    
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char resolved_path[MAX_PATH_LEN];
    int ret = resolve_path_with_dirs(path, data->lower_dir, data->upper_dir, resolved_path);
    if (ret != RESOLVE_IN_UPPER && ret != RESOLVE_IN_LOWER) {
        return -ENOENT;
    }
    
    fd = open(resolved_path, O_WRONLY);
    if (fd == -1) {
        return -errno;
    }
    
    res = pwrite(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }
    
    close(fd);
    return res;
}

static int unionfs_create(const char *path, mode_t mode,
                         struct fuse_file_info *fi) {
    (void) fi;
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    int fd;
    
    construct_path(data->upper_dir, path, upper_path);
    
    fd = open(upper_path, O_CREAT | O_WRONLY | O_EXCL, mode);
    if (fd == -1) {
        return -errno;
    }
    
    close(fd);
    return 0;
}

static int unionfs_open(const char *path, struct fuse_file_info *fi) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char resolved_path[MAX_PATH_LEN];
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    struct stat st;
    int fd;
    
    /* Resolve path to find actual location */
    int res = resolve_path_with_dirs(path, data->lower_dir, data->upper_dir, resolved_path);
    if (res != RESOLVE_IN_UPPER && res != RESOLVE_IN_LOWER && !(fi->flags & O_CREAT)) {
        return -ENOENT;
    }
    
    construct_path(data->upper_dir, path, upper_path);
    construct_path(data->lower_dir, path, lower_path);
    
    /* Copy-on-Write: If writing to a lower_dir file, copy it to upper_dir first */
    if ((fi->flags & (O_WRONLY | O_RDWR | O_APPEND)) && 
        stat(lower_path, &st) == 0 && stat(upper_path, &st) != 0) {
        
        /* File exists only in lower, need to copy to upper */
        char cmd[2 * MAX_PATH_LEN + 10];
        
        /* Ensure upper directory exists */
        char *upper_copy = strdup(upper_path);
        char *upper_dir_part = dirname(upper_copy);
        mkdir(upper_dir_part, 0755);
        free(upper_copy);
        
        /* Copy file */
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", lower_path, upper_path);
        if (system(cmd) != 0) {
            return -EIO;
        }
    }
    
    /* Open the file from the resolved location (upper takes precedence if exists) */
    fd = open(resolved_path, fi->flags, 0644);
    if (fd == -1) {
        return -errno;
    }
    
    close(fd);
    return 0;
}

static int unionfs_unlink(const char *path) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char resolved_path[MAX_PATH_LEN];
    char whiteout_path[MAX_PATH_LEN];
    char full_whiteout_path[MAX_PATH_LEN];
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    int res;
    
    resolve_path_with_dirs(path, data->lower_dir, data->upper_dir, resolved_path);
    
    construct_path(data->upper_dir, path, upper_path);
    construct_path(data->lower_dir, path, lower_path);
    get_whiteout_name(path, whiteout_path);
    construct_path(data->upper_dir, whiteout_path, full_whiteout_path);
    
    /* If file exists in upper directory, delete it */
    if (access(upper_path, F_OK) == 0) {
        res = unlink(upper_path);
        if (res == -1) {
            return -errno;
        }
    }
    
    /* If file exists in lower directory, create whiteout */
    if (access(lower_path, F_OK) == 0) {
        int fd = open(full_whiteout_path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            return -errno;
        }
        close(fd);
    }
    
    return 0;
}

static int unionfs_mkdir(const char *path, mode_t mode) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    
    construct_path(data->upper_dir, path, upper_path);
    
    int res = mkdir(upper_path, mode);
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static int unionfs_rmdir(const char *path) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    
    construct_path(data->upper_dir, path, upper_path);
    construct_path(data->lower_dir, path, lower_path);
    
    /* Try to remove upper directory first */
    int res = rmdir(upper_path);
    if (res == -1 && errno != ENOENT) {
        return -errno;
    }
    
    /* If directory exists in lower, create whiteout */
    struct stat st;
    if (stat(lower_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        char whiteout_path[MAX_PATH_LEN];
        char full_whiteout_path[MAX_PATH_LEN];
        get_whiteout_name(path, whiteout_path);
        construct_path(data->upper_dir, whiteout_path, full_whiteout_path);
        
        int fd = open(full_whiteout_path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            return -errno;
        }
        close(fd);
    }
    
    return 0;
}

static struct fuse_operations unionfs_oper = {
    .getattr = unionfs_getattr,
    .readdir = unionfs_readdir,
    .read = unionfs_read,
    .write = unionfs_write,
    .create = unionfs_create,
    .open = unionfs_open,
    .unlink = unionfs_unlink,
    .mkdir = unionfs_mkdir,
    .rmdir = unionfs_rmdir,
};

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <lower_dir> <upper_dir> <mount_point>\n", argv[0]);
        return 1;
    }
    
    struct mini_unionfs_state *data = malloc(sizeof(struct mini_unionfs_state));
    data->lower_dir = realpath(argv[1], NULL);
    data->upper_dir = realpath(argv[2], NULL);
    
    if (!data->lower_dir || !data->upper_dir) {
        fprintf(stderr, "Error: Invalid directories\n");
        return 1;
    }
    
    printf("Mini-UnionFS mounting:\n");
    printf("  Lower (read-only): %s\n", data->lower_dir);
    printf("  Upper (read-write): %s\n", data->upper_dir);
    printf("  Mount point: %s\n", argv[3]);
    
    /* Prepare FUSE arguments */
    char *fuse_argv[argc];
    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[3];
    fuse_argv[2] = "-f";
    fuse_argv[3] = "-d";
    
    int fuse_argc = 4;
    
    return fuse_main(fuse_argc, fuse_argv, &unionfs_oper, data);
}
