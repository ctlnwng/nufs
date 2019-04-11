#include "inode.h"
#include "pages.h"
#include "bitmap.h"
#include "util.h"

/*
typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
} inode;
*/

const int NUM_DIR_PTRS = 2;

inode*
get_inode(int inum)
{
    void* inodes = pages_get_page(1);
    return (inode*)(inodes + (sizeof(inode) * inum));
}

int
alloc_inode()
{
    void* ibm = get_inode_bitmap();

    for (int ii = 1; ii < 256; ++ii) {
        if (!bitmap_get(ibm, ii)) {
            bitmap_put(ibm, ii, 1);
            printf("+ alloc_inode() -> %d\n", ii);

            int pnum = alloc_page();
            
            inode* new_inode = get_inode(ii);
            new_inode->refs = 1;
            new_inode->mode = 0100644;
            new_inode->size = 0; 
            new_inode->ptrs[0] = pnum;;
            new_inode->ptrs[1] = -1;
            new_inode->iptr = 0;

            return ii;
        }
    }

    return -1;
}

void
free_inode(int inum)
{
    printf("+ free_inode(%d)\n", inum);
    void* ibm = get_inode_bitmap();
    bitmap_put(ibm, inum, 0);

    // also free page
    free_page(get_inode(inum)->ptrs[0]);
}

int
grow_inode(inode* node, int size)
{
    int new_size = node->size + size;
    
    int old_num_pages = bytes_to_pages(node->size);
    int new_num_pages = bytes_to_pages(new_size);
    int pages_to_grow = new_num_pages - old_num_pages;

    if (pages_to_grow > 0) {
        for (int ii = 0; ii < pages_to_grow; ++ii) {
            int pnum = alloc_page();
            
            if (old_num_pages + ii > NUM_DIR_PTRS) {
                // indirect stuff
            }
            else {
                inode->ptrs[old_num_pages + ii] = pnum;
            }

        }
    }
    else {
        node->size = new_size;
        return new_size;
    }
     
    return -1;
}

int
shrink_inode(inode* node, int size)
{
    int new_size = node->size - size;
    if (new_size >= 0) {
        node->size = new_size;
        return new_size;
    }
    else {
        return -1;
    }
}

//void print_inode(inode* node);
//int inode_get_pnum(inode* node, int fpn);


