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

#define MAX_PATH_LEN 4096
#define WHITEOUT_PREFIX ".wh."

/* Global state passed to FUSE */
struct mini_unionfs_state {
    char *lower_dir;
    char *upper_dir;
};

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

/* Helper function to construct full paths */
static void construct_path(const char *base_dir, const char *path, char *result) {
    if (path[0] == '/') {
        path++;
    }
    snprintf(result, MAX_PATH_LEN, "%s/%s", base_dir, path);
}

/* Get whiteout filename */
static void get_whiteout_name(const char *filename, char *whiteout_name) {
    char *fname_copy = strdup(filename);
    char *base = basename(fname_copy);
    char *dir = dirname(strdup(filename));
    
    if (dir[0] == '/' && dir[1] == '\0') {
        snprintf(whiteout_name, MAX_PATH_LEN, "/%s%s", WHITEOUT_PREFIX, base);
    } else {
        snprintf(whiteout_name, MAX_PATH_LEN, "%s/%s%s", dir, WHITEOUT_PREFIX, base);
    }
    free(fname_copy);
}

/* Check if a file is whiteouted in upper directory */
static int is_whiteouted(const char *path) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char whiteout_path[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];
    
    get_whiteout_name(path, whiteout_path);
    construct_path(data->upper_dir, whiteout_path, full_path);
    
    return (access(full_path, F_OK) == 0) ? 1 : 0;
}

/* Resolve path: returns which layer the file should be accessed from
 * Returns:
 *   - Full path if found in upper_dir (not whiteouted)
 *   - Full path if found in lower_dir (if not whiteouted)
 *   - -ENOENT if not found or whiteouted
 */
static int resolve_path(const char *path, char *resolved_path) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    struct stat st;
    
    /* Check if whiteouted */
    if (is_whiteouted(path)) {
        return -ENOENT;
    }
    
    /* Check upper directory first */
    construct_path(data->upper_dir, path, upper_path);
    if (stat(upper_path, &st) == 0) {
        strcpy(resolved_path, upper_path);
        return 0;
    }
    
    /* Check lower directory */
    construct_path(data->lower_dir, path, lower_path);
    if (stat(lower_path, &st) == 0) {
        strcpy(resolved_path, lower_path);
        return 0;
    }
    
    return -ENOENT;
}

/* FUSE Operations */

static int unionfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    char resolved_path[MAX_PATH_LEN];
    int res;
    
    res = resolve_path(path, resolved_path);
    if (res != 0) {
        return res;
    }
    
    res = lstat(resolved_path, stbuf);
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    struct mini_unionfs_state *data = UNIONFS_DATA;
    char upper_path[MAX_PATH_LEN];
    char lower_path[MAX_PATH_LEN];
    DIR *dp;
    struct dirent *de;
    int res = 0;
    
    /* Track which files we've already listed to avoid duplicates */
    char **listed_files = malloc(1000 * sizeof(char *));
    int file_count = 0;
    
    /* Read from upper directory first */
    construct_path(data->upper_dir, path, upper_path);
    dp = opendir(upper_path);
    
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            /* Skip whiteout files */
            if (strncmp(de->d_name, WHITEOUT_PREFIX, strlen(WHITEOUT_PREFIX)) == 0) {
                continue;
            }
            
            filler(buf, de->d_name, NULL, 0);
            listed_files[file_count++] = strdup(de->d_name);
        }
        closedir(dp);
    }
    
    /* Read from lower directory, skip duplicates and whiteouted files */
    construct_path(data->lower_dir, path, lower_path);
    dp = opendir(lower_path);
    
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            int is_duplicate = 0;
            
            /* Check if already listed from upper */
            for (int i = 0; i < file_count; i++) {
                if (strcmp(listed_files[i], de->d_name) == 0) {
                    is_duplicate = 1;
                    break;
                }
            }
            
            if (!is_duplicate) {
                /* Check if whiteouted */
                char full_path[MAX_PATH_LEN];
                snprintf(full_path, MAX_PATH_LEN, "%s/%s", path, de->d_name);
                if (!is_whiteouted(full_path)) {
                    filler(buf, de->d_name, NULL, 0);
                }
            }
        }
        closedir(dp);
    }
    
    /* Cleanup */
    for (int i = 0; i < file_count; i++) {
        free(listed_files[i]);
    }
    free(listed_files);
    
    return res;
}

static int unionfs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    int fd;
    int res;
    
    char resolved_path[MAX_PATH_LEN];
    int ret = resolve_path(path, resolved_path);
    if (ret != 0) {
        return ret;
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
    int fd;
    int res;
    
    char resolved_path[MAX_PATH_LEN];
    int ret = resolve_path(path, resolved_path);
    if (ret != 0) {
        return ret;
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
    
    /* Resolve initial path */
    int res = resolve_path(path, resolved_path);
    if (res != 0 && !(fi->flags & O_CREAT)) {
        return res;
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
    
    /* Try to open from upper directory */
    fd = open(upper_path, fi->flags, 0644);
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
    
    resolve_path(path, resolved_path);
    
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
