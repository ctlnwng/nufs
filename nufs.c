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
    inode* root_inode = get_inode(0);

    if (strcmp(path, "/") == 0) {
        st->st_mode = root_inode->mode; // directory
        st->st_size = root_inode->size;
        st->st_uid = getuid();
    }
    else if (directory_lookup(get_inode(0), path) != -1) {
        inode* file_inode = get_inode(directory_lookup(root_inode, path));
        
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

    rv = nufs_getattr("/", &st);
    assert(rv == 0);
    filler(buf, ".", &st, 0);

    inode* root_inode = get_inode(0);
    void* directory = pages_get_page(root_inode->ptrs[0]);

    // write all files in root directory to buffer
    for (int ii = 0; ii < root_inode->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);

        rv = nufs_getattr(entry->name, &st);
        assert(rv == 0);
        filler(buf, entry->name + 1, &st, 0);
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
 
    int inum = alloc_inode();
    inode* root_inode = get_inode(0);
    
    if (directory_put(root_inode, path, inum) != -1) {
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
    rv = directory_delete(get_inode(0), path);    

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
    int rv = -1;
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
    int rv;
    rv = directory_rename_entry(get_inode(0), from, to);

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
    int rv = 0;
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
    int inum = directory_lookup(get_inode(0), path);
    inode* inode = get_inode(inum);
    int pnum = inode->ptrs[0];

    void* page = pages_get_page(pnum);
    memcpy(buf, (char*)page, inode->size);

    int rv = inode->size;

    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = -ENOENT;

    inode* root_inode = get_inode(0);
    int inum = directory_lookup(root_inode, path);

    inode* file_inode = get_inode(inum);
    int pnum = file_inode->ptrs[0];
    void* page = pages_get_page(pnum);

    memcpy(page + file_inode->size, buf, size);

    if (grow_inode(file_inode, size) != -1) {
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
