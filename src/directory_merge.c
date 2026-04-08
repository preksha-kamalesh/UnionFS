#include "directory_merge.h"
#include "path_resolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

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
int unionfs_getattr(const char *path, struct stat *stbuf,
                   struct fuse_file_info *fi) {
    char resolved_path[MAX_PATH_LEN];
    int ret;
    
    /* Resolve path to actual location */
    ret = resolve_path(path, resolved_path);
    
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
int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    (void) offset; /* Unused */
    (void) fi;     /* Unused */
    
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    DIR *dp;
    struct dirent *de;
    int res = 0;
    
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
            
            if (is_whiteouted(full_path)) {
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
    
    return res;
}
