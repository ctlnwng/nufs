// based on cs3650 starter code

#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>

static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}

static int
min(int x, int y)
{
    return (x < y) ? x : y;
}

static int
max(int x, int y)
{
    return (x > y) ? x : y;
}

static int
clamp(int x, int v0, int v1)
{
    return max(v0, min(x, v1));
}

static int
bytes_to_pages(int bytes)
{
    int quo = bytes / 4096;
    int rem = bytes % 4096;
    if (rem == 0) {
        return quo;
    }
    else {
        return quo + 1;
    }
}

static void
join_to_path(char* buf, char* item)
{
    int nn = strlen(buf);
    if (buf[nn - 1] != '/') {
        strcat(buf, "/");
    }
    strcat(buf, item);
}

static void
get_parent_path(const char* buf, char parent_path[])
{
    memset(parent_path, 0, 48);
    int nn = strlen(buf);

    for (int ii = nn - 1; ii >= 0; --ii) {
        if (buf[ii] == '/' && ii != 0) {
            memcpy(parent_path, buf, ii);
            parent_path[ii] = '\0';
            return;
        }
        if (ii == 0) {
            memcpy(parent_path, buf, 1);
            parent_path[1] = '\0';
        }
    }
}

static void
get_relative_path(const char* buf, char relative_path[])
{
    memset(relative_path, 0, 48);
    int nn = strlen(buf);

    for (int ii = nn - 1; ii >= 0; --ii) {
        if (buf[ii] == '/') {
            memcpy(relative_path, (buf + ii + 1), (nn - ii - 1));
            relative_path[nn - ii] = '\0';
            return;
        }
    }
}

#endif
