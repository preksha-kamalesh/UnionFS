#!/bin/bash

FUSE_BINARY="./mini_unionfs"
TEST_DIR="./unionfs_test_env"
LOWER_DIR="$TEST_DIR/lower"
UPPER_DIR="$TEST_DIR/upper"
MOUNT_DIR="$TEST_DIR/mnt"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Helper function
run_test() {
    local test_num=$1
    local test_name=$2
    local test_func=$3
    
    echo -n "Test $test_num: $test_name... "
    if $test_func; then
        echo -e "${GREEN}PASSED${NC}"
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        return 1
    fi
}

# Test functions
test_visibility() {
    grep -q "base_only_content" "$MOUNT_DIR/base.txt" 2>/dev/null
    return $?
}

test_copy_on_write() {
    echo "modified_content" >> "$MOUNT_DIR/base.txt" 2>/dev/null
    
    # Check that modification appears in mounted view
    if ! grep -q "modified_content" "$MOUNT_DIR/base.txt" 2>/dev/null; then
        return 1
    fi
    
    # Check that modification is in upper directory
    if ! grep -q "modified_content" "$UPPER_DIR/base.txt" 2>/dev/null; then
        return 1
    fi
    
    # Check that lower directory is untouched
    if grep -q "modified_content" "$LOWER_DIR/base.txt" 2>/dev/null; then
        return 1
    fi
    
    return 0
}

test_whiteout() {
    rm "$MOUNT_DIR/delete_me.txt" 2>/dev/null
    
    # File should not be visible in mount
    if [ -f "$MOUNT_DIR/delete_me.txt" ]; then
        return 1
    fi
    
    # File should still exist in lower
    if [ ! -f "$LOWER_DIR/delete_me.txt" ]; then
        return 1
    fi
    
    # Whiteout file should exist in upper
    if [ ! -f "$UPPER_DIR/.wh.delete_me.txt" ]; then
        return 1
    fi
    
    return 0
}

test_file_creation() {
    echo "new_content" > "$MOUNT_DIR/new_file.txt" 2>/dev/null
    
    # File should exist in upper directory
    if [ ! -f "$UPPER_DIR/new_file.txt" ]; then
        return 1
    fi
    
    # Content should match
    if ! grep -q "new_content" "$UPPER_DIR/new_file.txt" 2>/dev/null; then
        return 1
    fi
    
    return 0
}

test_readdir() {
    local files=$(ls -1 "$MOUNT_DIR" 2>/dev/null | wc -l)
    
    # Should show at least base.txt and new_file.txt
    if [ "$files" -lt 2 ]; then
        return 1
    fi
    
    return 0
}

test_mkdir() {
    mkdir "$MOUNT_DIR/testdir" 2>/dev/null
    
    # Should exist in upper directory
    if [ ! -d "$UPPER_DIR/testdir" ]; then
        return 1
    fi
    
    return 0
}

# Main test suite
echo -e "${YELLOW}Starting Mini-UnionFS Test Suite...${NC}\n"

# Check if binary exists
if [ ! -f "$FUSE_BINARY" ]; then
    echo -e "${RED}Error: $FUSE_BINARY not found. Please compile first with 'make'.${NC}"
    exit 1
fi

# Setup
echo "Setting up test environment..."
rm -rf "$TEST_DIR" 2>/dev/null
mkdir -p "$LOWER_DIR" "$UPPER_DIR" "$MOUNT_DIR"

echo "base_only_content" > "$LOWER_DIR/base.txt"
echo "to_be_deleted" > "$LOWER_DIR/delete_me.txt"

# Mount the filesystem
echo "Mounting Mini-UnionFS..."
$FUSE_BINARY "$LOWER_DIR" "$UPPER_DIR" "$MOUNT_DIR" >/dev/null 2>&1 &
FUSE_PID=$!
sleep 2

# Check if mount was successful
if [ ! -d "$MOUNT_DIR" ] || [ -z "$(ls -A $MOUNT_DIR 2>/dev/null)" ]; then
    if [ ! -z "$(ls -A $MOUNT_DIR 2>/dev/null)" ] || grep -q "base_only_content" "$MOUNT_DIR/base.txt" 2>/dev/null; then
        :  # Mount successful
    else
        echo -e "${RED}Mount failed${NC}"
        kill $FUSE_PID 2>/dev/null
        rm -rf "$TEST_DIR"
        exit 1
    fi
fi

echo -e "\n${YELLOW}Running Tests:${NC}\n"

# Run tests
passed=0
failed=0

run_test 1 "Layer Visibility" test_visibility && ((passed++)) || ((failed++))
run_test 2 "Copy-on-Write" test_copy_on_write && ((passed++)) || ((failed++))
run_test 3 "Whiteout Mechanism" test_whiteout && ((passed++)) || ((failed++))
run_test 4 "File Creation" test_file_creation && ((passed++)) || ((failed++))
run_test 5 "Directory Listing" test_readdir && ((passed++)) || ((failed++))
run_test 6 "Directory Creation" test_mkdir && ((passed++)) || ((failed++))

# Teardown
echo -e "\n${YELLOW}Cleaning up...${NC}"
fusermount -u "$MOUNT_DIR" 2>/dev/null || umount "$MOUNT_DIR" 2>/dev/null
sleep 1
kill $FUSE_PID 2>/dev/null
wait $FUSE_PID 2>/dev/null

rm -rf "$TEST_DIR"

# Summary
echo -e "\n${YELLOW}Test Summary:${NC}"
echo -e "Passed: ${GREEN}$passed${NC}"
echo -e "Failed: ${RED}$failed${NC}"

if [ $failed -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed.${NC}"
    exit 1
fi
