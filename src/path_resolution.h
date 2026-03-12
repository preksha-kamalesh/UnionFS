#ifndef PATH_RESOLUTION_H
#define PATH_RESOLUTION_H

#include <sys/stat.h>

#define MAX_PATH_LEN 4096
#define WHITEOUT_PREFIX ".wh."

/* Global state structure passed to FUSE */
struct mini_unionfs_state {
    char *lower_dir;    /* Read-only base layer */
    char *upper_dir;    /* Read-write container layer */
};

/* Path resolution result codes */
typedef enum {
    RESOLVE_IN_UPPER = 0,      /* File found in upper directory */
    RESOLVE_IN_LOWER = 1,      /* File found in lower directory only */
    RESOLVE_WHITEOUTED = -1,   /* File is whiteouted (deleted) */
    RESOLVE_NOT_FOUND = -2,    /* File not found in any layer */
} resolve_result_t;

/**
 * Construct full path from base directory and relative path
 *
 * @param base_dir  Base directory path
 * @param path      Relative path within filesystem
 * @param result    Output buffer for full path (MAX_PATH_LEN)
 */
void construct_path(const char *base_dir, const char *path, char *result);

/**
 * Get the whiteout filename for a given path
 * Example: /config.txt -> /.wh.config.txt
 *
 * @param path              Original file path
 * @param whiteout_name     Output buffer for whiteout path (MAX_PATH_LEN)
 */
void get_whiteout_name(const char *path, char *whiteout_name);

/**
 * Check if a file is whiteouted in the upper directory
 *
 * @param path      Path to check
 * @return          1 if whiteouted, 0 otherwise
 */
int is_whiteouted(const char *path);

/**
 * Resolve file location across layers
 *
 * Implements the core precedence logic:
 * 1. If .wh.<filename> exists in upper_dir -> return RESOLVE_WHITEOUTED
 * 2. If file exists in upper_dir -> return RESOLVE_IN_UPPER
 * 3. If file exists in lower_dir -> return RESOLVE_IN_LOWER
 * 4. Otherwise -> return RESOLVE_NOT_FOUND
 *
 * @param path              Path to resolve
 * @param resolved_path     Output buffer for full resolved path (MAX_PATH_LEN)
 * @return                  resolve_result_t indicating location and status
 */
resolve_result_t resolve_path(const char *path, char *resolved_path);

/**
 * Resolve path with explicit upper/lower directory paths
 * Used internally for testing and debugging
 *
 * @param path              Path to resolve
 * @param lower_dir_path    Lower directory path
 * @param upper_dir_path    Upper directory path
 * @param resolved_path     Output buffer for full resolved path
 * @return                  resolve_result_t indicating location and status
 */
resolve_result_t resolve_path_with_dirs(const char *path,
                                       const char *lower_dir_path,
                                       const char *upper_dir_path,
                                       char *resolved_path);

#endif /* PATH_RESOLUTION_H */
