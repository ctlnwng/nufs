// based on cs3650 starter code

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <bsd/string.h>
#include <assert.h>
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "util.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

// implementation for: man 2 access
// checks if a file exists
int
nufs_access(const char *path, int mask)
{
    int rv = 0;
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
    int rv = 0;
    char* parent_path = get_parent_path(path);
    int parent_inum = tree_lookup(parent_path);
    free(parent_path);

    if (parent_inum == -1) {
        return -ENOENT;
    }

    inode* node = get_inode(parent_inum);

    if (strcmp(path, "/") == 0) {
        st->st_mode = node->mode; // directory
        st->st_size = node->size;
        st->st_uid = getuid();
    }
    else if (directory_lookup(node, path) != -1) {
        inode* file_inode = get_inode(directory_lookup(node, path));
        
        st->st_mode = file_inode->mode; // regular file
        st->st_size = file_inode->size;
        st->st_uid = getuid();
    }
    else {
        rv = -ENOENT;
    }

    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    struct stat st;
    int rv;

    rv = nufs_getattr(path, &st);
    assert(rv == 0);
    filler(buf, ".", &st, 0);

    char* parent_path = get_parent_path(path);
    inode* node = get_inode(tree_lookup(parent_path));
    free(parent_path);

    // TODO: look for page in nested for loop
    void* directory = pages_get_page(node->ptrs[0]);
    
    // write all files in root directory to buffer
    for (int ii = 0; ii < node->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);

        rv = nufs_getattr(entry->name, &st);
        assert(rv == 0);
        filler(buf, get_relative_path(entry->name), &st, 0);
    }

    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int rv = -ENOENT;
    struct stat st;
 
    int inum = alloc_inode(mode);
    
    char* parent_path = get_parent_path(path);
    inode* node = get_inode(tree_lookup(parent_path));
    free(parent_path);

    if (directory_put(node, path, inum) != -1) {
        rv = nufs_getattr(path, &st);
    }

    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    int rv = nufs_mknod(path, mode | 040000, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_unlink(const char *path)
{
    int rv;
    struct stat st;
    rv = nufs_getattr(path, &st);

    if (rv == -ENOENT) {
        return rv;
    }

    char* parent_path = get_parent_path(path);
    inode* node = get_inode(tree_lookup(parent_path));
    free(parent_path);

    rv = directory_delete(node, path);    

    if (rv == -1) {
        rv = -ENOENT;
    }

    printf("unlink(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_link(const char *from, const char *to)
{
    int rv = -1;
    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{
    int rv;
    struct stat st;
    rv = nufs_getattr(path, &st);

    if (rv == -ENOENT) {
        return rv;
    }

    int inum = tree_lookup(path);
    inode* dir_node = get_inode(inum);
    
    // TODO: nested for loop for dir pages
    void* directory = pages_get_page(dir_node->ptrs[0]);

    // recursively remove children
    for (int ii = 0; ii < dir_node->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);
        inode* entry_node = get_inode(entry->inum);

        if (entry_node->mode == 0100644) {
            nufs_unlink(entry->name);
        }
        else {
            nufs_rmdir(entry->name);
        }
    }

    // find parent inode
    char* parent_path = get_parent_path(path);
    inode* node = get_inode(tree_lookup(parent_path));
    free(parent_path);

    // delete dir from parent
    rv = directory_delete(node, path);
    
    if (rv == -1) {
        rv = -ENOENT;
    }

    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
    int rv;
    
    char* parent_path = get_parent_path(from);
    inode* node = get_inode(tree_lookup(parent_path));
    free(parent_path);

    rv = directory_rename_entry(node, from, to);

    if (rv == -1) {
        rv = -ENOENT;
    }

    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
    int rv = -1;
    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
    struct stat st;
    int rv = nufs_getattr(path, &st);

    if (rv == -ENOENT) {
        return rv;
    }
    
    int inum = directory_lookup(get_inode(0), path);
    inode* inode = get_inode(inum);
    rv = shrink_inode(inode, size);

    printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
    return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    int rv = 0;
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct stat st;
    int rv = nufs_getattr(path, &st);
    if (rv == -ENOENT) {
        return rv;
    }
     
    inode* node = get_inode(tree_lookup(path));
    
    size = size > node->size ? node->size : size;
    read_from_file(node, buf, size, offset);

    rv = size;

    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = -ENOENT;
    
    inode* node = get_inode(tree_lookup(path));

    // what if offset is > file_inode size
    if (grow_inode(node, (offset + size - node->size)) != -1) {
        write_to_file(node, buf, size, offset);
        rv = size;
    }

    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = 0;
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->link     = nufs_link;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    //storage_init(argv[--argc]);
    pages_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}

