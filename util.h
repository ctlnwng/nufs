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

static char*
get_parent_path(const char* buf)
{
    int nn = strlen(buf);
    char* trimmed_path = malloc(sizeof(char) * 48);

    for (int ii = nn - 1; ii >= 0; --ii) {
        if (buf[ii] == '/' && ii != 0) {
            memcpy(trimmed_path, buf, ii);
            return trimmed_path;
        }
        if (ii == 0) {
            memcpy(trimmed_path, buf, 1);
        }
    }

    return trimmed_path;
}

static char*
get_relative_path(const char* buf)
{
    int nn = strlen(buf);
    char* relative_path = malloc(sizeof(char) * 48);

    for (int ii = nn - 1; ii >= 0; --ii) {
        if (buf[ii] == '/') {
            memcpy(relative_path, (buf + ii), nn - ii);
            return relative_path;
        }
    }

    return relative_path;
}

#endif
