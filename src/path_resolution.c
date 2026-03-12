#include "path_resolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fuse.h>

/**
 * Construct full path from base directory and relative path
 *
 * Handles relative paths by stripping leading '/' from the input path
 * and appending to the base directory.
 *
 * Example:
 *   base_dir = "/mnt/lower"
 *   path = "/file.txt"
 *   result = "/mnt/lower/file.txt"
 */
void construct_path(const char *base_dir, const char *path, char *result) {
    const char *relative_path = path;
    
    /* Strip leading slash if present */
    if (path[0] == '/') {
        relative_path = path + 1;
    }
    
    /* Construct the full path */
    snprintf(result, MAX_PATH_LEN, "%s/%s", base_dir, relative_path);
}

/**
 * Get the whiteout filename for a given path
 *
 * Whiteout files use the .wh. prefix and are placed in the same
 * directory as the file they represent.
 *
 * Examples:
 *   /config.txt       -> /.wh.config.txt
 *   /etc/passwd       -> /etc/.wh.passwd
 *   /dir/subdir/file  -> /dir/subdir/.wh.file
 */
void get_whiteout_name(const char *path, char *whiteout_name) {
    char *path_copy = strdup(path);
    char *filename = basename(path_copy);
    char *dir_copy = strdup(path);
    char *dirpart = dirname(dir_copy);
    
    /* Build whiteout path */
    if (strlen(dirpart) == 1 && dirpart[0] == '/') {
        /* File in root directory */
        snprintf(whiteout_name, MAX_PATH_LEN, "/%s%s", WHITEOUT_PREFIX, filename);
    } else {
        /* File in subdirectory */
        snprintf(whiteout_name, MAX_PATH_LEN, "%s/%s%s", dirpart, WHITEOUT_PREFIX, filename);
    }
    
    free(path_copy);
    free(dir_copy);
}

/**
 * Check if a file is whiteouted in the upper directory
 *
 * A whiteout file is a marker that indicates the file should be
 * hidden even if it exists in a lower layer.
 *
 * @param upper_dir_path Upper directory path
 * @param path           File path to check
 * @return               1 if whiteouted, 0 otherwise
 */
int is_whiteouted_internal(const char *upper_dir_path, const char *path) {
    char whiteout_path[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];
    
    /* Get whiteout filename */
    get_whiteout_name(path, whiteout_path);
    
    /* Construct full path in upper directory */
    construct_path(upper_dir_path, whiteout_path, full_path);
    
    /* Check if whiteout file exists */
    return (access(full_path, F_OK) == 0) ? 1 : 0;
}

/**
 * Check if a file is whiteouted (wrapper using FUSE context)
 * This version uses UNIONFS_DATA and should only be called from FUSE callbacks
 */
int is_whiteouted(const char *path) {
    /* Note: This function would use UNIONFS_DATA if FUSE context available
     * For now, use the version below instead in main FUSE module
     */
    return 0;
}

/**
 * Core path resolution logic
 *
 * Implements the Union FS layer precedence:
 * Priority 1: Whiteout check (file is intentionally deleted)
 * Priority 2: Upper directory (user modifications take precedence)
 * Priority 3: Lower directory (base/read-only layer)
 * Priority 4: Not found (file doesn't exist)
 *
 * This version requires explicit directory parameters (preferred for modularity)
 *
 * @return RESOLVE_IN_UPPER, RESOLVE_IN_LOWER, RESOLVE_WHITEOUTED, or RESOLVE_NOT_FOUND
 */
resolve_result_t resolve_path(const char *path, char *resolved_path) {
    /* This is a legacy stub - actual implementation in directory_merge.c
     * which has access to UNIONFS_DATA
     */
    return RESOLVE_NOT_FOUND;
}

/**
 * Resolve path with explicit directory parameters
 *
 * Useful for testing and when you don't have UNIONFS_DATA available.
 * Same logic as resolve_path() but takes directories as parameters.
 */
resolve_result_t resolve_path_with_dirs(const char *path,
                                       const char *lower_dir_path,
                                       const char *upper_dir_path,
                                       char *resolved_path) {
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    char whiteout_path[MAX_PATH_LEN];
    char full_whiteout_path[MAX_PATH_LEN];
    struct stat st;
    
    /* Step 1: Check whiteout */
    get_whiteout_name(path, whiteout_path);
    construct_path(upper_dir_path, whiteout_path, full_whiteout_path);
    if (access(full_whiteout_path, F_OK) == 0) {
        return RESOLVE_WHITEOUTED;
    }
    
    /* Step 2: Check upper directory */
    construct_path(upper_dir_path, path, upper_path);
    if (stat(upper_path, &st) == 0) {
        strcpy(resolved_path, upper_path);
        return RESOLVE_IN_UPPER;
    }
    
    /* Step 3: Check lower directory */
    construct_path(lower_dir_path, path, lower_path);
    if (stat(lower_path, &st) == 0) {
        strcpy(resolved_path, lower_path);
        return RESOLVE_IN_LOWER;
    }
    
    /* Step 4: Not found */
    return RESOLVE_NOT_FOUND;
}
