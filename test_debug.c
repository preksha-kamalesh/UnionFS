#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/path_resolution.h"

int main() {
    char resolved[MAX_PATH_LEN];
    
    // Test construct_path
    char upper[] = "/tmp/upper";
    char lower[] = "/tmp/lower";
    char path[] = "/testfile.txt";
    
    printf("Testing construct_path:\n");
    construct_path(lower, path, resolved);
    printf("  lower path: %s\n", resolved);
    
    construct_path(upper, path, resolved);
    printf("  upper path: %s\n", resolved);
    
    // Test resolve_path_with_dirs
    printf("\nTesting resolve_path_with_dirs:\n");
    int ret = resolve_path_with_dirs(path, lower, upper, resolved);
    printf("  Result: %d (0=UPPER, 1=LOWER, -1=WHITEOUTED, -2=NOTFOUND)\n", ret);
    printf("  Resolved path: %s\n", resolved);
    
    // Test get_whiteout_name
    printf("\nTesting get_whiteout_name:\n");
    char whiteout[MAX_PATH_LEN];
    get_whiteout_name(path, whiteout);
    printf("  Whiteout name: %s\n", whiteout);
    
    // Test is_whiteouted_internal
    printf("\nTesting is_whiteouted_internal:\n");
    int ws = is_whiteouted_internal(upper, path);
    printf("  Is whiteouted: %d\n", ws);
    
    return 0;
}
