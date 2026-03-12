#ifndef DIRECTORY_MERGE_H
#define DIRECTORY_MERGE_H

#include <fuse.h>
#include "path_resolution.h"

/**
 * Get file attributes using path resolution
 *
 * Implements merged filesystem view by:
 * 1. Resolving path to actual location (upper or lower)
 * 2. Getting stat information from that location
 * 3. Handling whiteouts and missing files
 *
 * @param path   Virtual filesystem path
 * @param stbuf  Output buffer for stat structure
 * @param fi     FUSE file info (optional, can be NULL)
 * @return       0 on success, -errno on error
 */
int unionfs_getattr(const char *path, struct stat *stbuf,
                   struct fuse_file_info *fi);

/**
 * List merged directory contents
 *
 * Merges directory listings from both layers:
 * 1. Lists all files from upper directory first
 * 2. Adds files from lower directory (avoiding duplicates)
 * 3. Filters out whiteout files (.wh.* files)
 * 4. Handles deleted files properly (hidden by whiteouts)
 *
 * Example:
 *   Lower: [base.txt, config, readme.md]
 *   Upper: [config, temp.txt, .wh.readme.md]
 *   Result: [base.txt, config (from upper), temp.txt]
 *   (readme.md is hidden by .wh.readme.md)
 *
 * @param path   Virtual directory path
 * @param buf    Buffer to fill directory entries into
 * @param filler Function to call for each directory entry
 * @param offset Current offset in loop (ignored in simple implementation)
 * @param fi     FUSE file info
 * @return       0 on success, -errno on error
 */
int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi);

#endif /* DIRECTORY_MERGE_H */
