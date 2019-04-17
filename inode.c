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
    time_t atime; // access time
    time_t mtime; // modified time
    time_t ctime; // changed time
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
alloc_inode(int mode)
{
    void* ibm = get_inode_bitmap();

    for (int ii = 1; ii < 256; ++ii) {
        if (!bitmap_get(ibm, ii)) {
            bitmap_put(ibm, ii, 1);
            printf("+ alloc_inode() -> %d\n", ii);

            inode* new_inode = get_inode(ii);
            new_inode->refs = 1;
            new_inode->mode = mode;
            new_inode->size = 0;
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

    // also free pages
    inode* node = get_inode(inum);
    int num_pages = bytes_to_pages(node->size);
    
    for (int ii = 0; ii < num_pages; ++ii) {
        if (ii < NUM_DIR_PTRS) {
            free_page(node->ptrs[ii]);
        }
        else {
            int* iptr_page = pages_get_page(node->iptr);
            int pnum = iptr_page[ii - NUM_DIR_PTRS];
            free_page(pnum);
        }
    }

    if (num_pages > NUM_DIR_PTRS) {
        free_page(node->iptr);
    }
}

int
grow_inode(inode* node, int size)
{
    int new_size = node->size + size;
    
    int old_num_pages = bytes_to_pages(node->size);
    int new_num_pages = bytes_to_pages(new_size);
    int pages_to_grow = new_num_pages - old_num_pages;

    if (pages_to_grow > 0) {
        int pnum = alloc_page();
        if (pnum == -1) {
            return -1;
        }
        
        if (new_num_pages > NUM_DIR_PTRS) {
            int iptr_idx = (new_num_pages - NUM_DIR_PTRS) - 1;
            int rv = grow_iptr(&node->iptr, pnum, iptr_idx);
            if (rv == -1) {
                return -1;
            }
        }
        else {
            node->ptrs[new_num_pages - 1] = pnum;
        }
    }

    node->size = new_size;
    return new_size; 
}

int
shrink_inode(inode* node, int size)
{
    // potentially memset truncated memory to 0
    int new_size = node->size - size;

    if (new_size < 0) {
        return -1;
    }

    int old_num_pages = bytes_to_pages(node->size);
    int new_num_pages = bytes_to_pages(new_size);

    for (int ii = old_num_pages - 1; ii > new_num_pages - 1; --ii) {
        if(ii < NUM_DIR_PTRS) {
            free_page(node->ptrs[ii]);
            node->ptrs[ii] = 0;
        }
        else {
            int* iptr_page = pages_get_page(node->iptr);
            int* cur_page = iptr_page + (ii - NUM_DIR_PTRS);

            free_page(*cur_page);
            *cur_page = 0;
        }
    }

    node->size = new_size;
    return new_size;
}

int
grow_iptr(int* iptr, int pnum, int iptr_idx)
{
    if (*iptr == 0) {
        *iptr = alloc_page();
    }
    
    if (*iptr == -1 || (iptr_idx * sizeof(int)) >= PAGE_SIZE) {
        return -1;
    }

    int* iptr_page = pages_get_page(*iptr);
    *(iptr_page + iptr_idx) = pnum;

    return 0;
}

void
write_to_file(inode* node, const char *buf, size_t size, off_t offset)
{
    int cur_page_idx;
    void* cur_page;

    while(size > 0) {
        cur_page_idx = offset / PAGE_SIZE;
    
        if (cur_page_idx < NUM_DIR_PTRS) {
            cur_page = pages_get_page(node->ptrs[cur_page_idx]);
        }
        else {
            int* iptr_page = pages_get_page(node->iptr);
            cur_page = pages_get_page(*(iptr_page + (cur_page_idx - NUM_DIR_PTRS)));
        }

        int offset_in_page = offset - (PAGE_SIZE * cur_page_idx);
        int bytes_left_in_page = PAGE_SIZE - offset_in_page;
        int bytes_to_write = size > bytes_left_in_page ? bytes_left_in_page : size;
        memcpy(cur_page + offset_in_page, buf, bytes_to_write);

        size -= bytes_to_write;
        offset += bytes_to_write;
    }
}

void
read_from_file(inode* node, char *buf, size_t size, off_t offset)
{
    int cur_page_idx = 0;
    void* cur_page = 0;
    off_t buf_offset = 0;

    while(size > 0) {
        cur_page_idx = offset / PAGE_SIZE;

        if (cur_page_idx < NUM_DIR_PTRS) {
            cur_page = pages_get_page(node->ptrs[cur_page_idx]);
        }
        else {
            int* iptr_page = pages_get_page(node->iptr);
            cur_page = pages_get_page(*(iptr_page + (cur_page_idx - NUM_DIR_PTRS)));
        }

        int offset_in_page = offset - (PAGE_SIZE * cur_page_idx);
        int bytes_left_in_page = PAGE_SIZE - offset_in_page;
        int bytes_to_read = size > bytes_left_in_page ? bytes_left_in_page : size;
        memcpy(buf + buf_offset, (char*)(cur_page + offset_in_page), bytes_to_read);
        
        buf_offset += bytes_to_read;
        size -= bytes_to_read;
        offset += bytes_to_read;
    }
}



//void print_inode(inode* node);
//int inode_get_pnum(inode* node, int fpn);


