#include <string.h>
#include <assert.h>
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "util.h"
#include "slist.h"

const int MAX_NAME_LEN = 48;

/*
typedef struct dirent {
    char name[DIR_NAME];
    int  inum;
    char _reserved[12];
} dirent;
*/

// returns inode num of given path, -1 if path doesn't exist
int directory_lookup(inode* dd, const char* name)
{
    void* directory = pages_get_page(dd->ptrs[0]);

    for (int ii = 0; ii < dd->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);

        if (streq(entry->name, name)) {
            return entry->inum;
        }
    }

    return -1;
}

// returns 0 if successful, -1 if 'from' path doesn't exist
int directory_rename_entry(inode* dd, const char* from, const char* to)
{
    void* directory = pages_get_page(dd->ptrs[0]);

    for (int ii = 0; ii < dd->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);
        char* entry_name = entry->name;

        if (streq(entry_name, from)) {
            memset(entry_name, '\0', MAX_NAME_LEN);
            strcpy(entry_name, to);
            return 0;
        }
    }

    return -1;
}


int
directory_put(inode* dd, const char* name, int inum)
{
    int rv = -1;
    if (grow_inode(dd, sizeof(dirent)) != -1) {
        void* directory = pages_get_page(dd->ptrs[0]);
    
        dirent* entry = (dirent*)(directory + (dd->size - sizeof(dirent)));
        char* entry_name = entry->name;

        memset(entry_name, '\0', MAX_NAME_LEN);
        strcpy(entry_name, name);
        entry->inum = inum;

        rv = 0;
    }
    
    return rv;
}

int
directory_delete(inode* dd, const char* name)
{
    int rv = -1;
    int file_pos = -1;
    int file_inum = -1;

    void* directory = pages_get_page(dd->ptrs[0]);
    
    for (int ii = 0; ii < dd->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);
        
        if (streq(entry->name, name)) {
            file_pos = ii;
            file_inum = entry->inum;
            break;
        }
    }
/*
    if (file_inum == -1) {
        return rv;
    }
    
    free_inode(file_inum);
*/
    for (int ii = file_pos + sizeof(dirent); ii < dd->size; ii += sizeof(dirent)) {
        dirent* entry = (dirent*)(directory + ii);
        dirent* prev_entry = (dirent*)(directory + ii - sizeof(dirent));
        
        memset(prev_entry->name, '\0', MAX_NAME_LEN);
        strcpy(prev_entry->name, entry->name);
        prev_entry->inum = entry->inum;
    }

    if (shrink_inode(dd, sizeof(dirent)) != -1) {
        rv = 0;
    }

    return rv;
}

slist* directory_list(const char* path) {
    slist* list = s_cons(path, 0);

    for (int ii = strlen(path) - 1; ii >= 0; --ii) {
        if (ii != 0 && path[ii] == '/') {
            char str[MAX_NAME_LEN];
            memcpy(str, path, ii);
            str[ii] = '\0';
            list = s_cons(str, list);
        }
    }

    return list;
}

int tree_lookup(const char* path) {
    if (strcmp(path, "/") == 0) {
        return 0;
    }

    slist* dir_list = directory_list(path);
    slist* list_start = dir_list;

    int inum = 0;
    while (dir_list != 0 && inum != -1) {
        inode* cur_dir = get_inode(inum);
        inum = directory_lookup(cur_dir, dir_list->data);
        dir_list = dir_list->next;
    }
    
    s_free(list_start);
    return inum;
}

//void print_directory(inode* dd);
//void directory_init(int pnum, char* name);
