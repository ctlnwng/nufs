// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "pages.h"

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode();
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);
int grow_iptr(int* iptr, int pnum, int num_iptrs);
void write_to_file(inode* node, const char *buf, size_t size, off_t offset);
void read_from_file(inode* node, char *buf, size_t size, off_t offset);

#endif
