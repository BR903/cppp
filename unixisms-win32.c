/* unixisms.c: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <string.h>
#include <windows.h>
#include "unixisms.h"

/* Buffer holding the saved directory.
 */
static char saveddir[MAX_PATH];

/* Changes the current directory.
 */
int changedir(char const *name)
{
    return SetCurrentDirectory(name);
}

/* Opens a file descriptor on the current directory.
 */
int savedir(void)
{
    return GetCurrentDirectory(sizeof saveddir, saveddir) > 0;
}

/* Uses the saved file description to change the directory back.
 */
int restoredir(void)
{
    return SetCurrentDirectory(saveddir);
}

/* Returns true if the filename is a directory.
 */
int fileisdir(char const *name)
{
    DWORD attrs;

    attrs = GetFileAttributes(name);
    return attrs & FILE_ATTRIBUTE_DIRECTORY;
}

/* Returns a pointer to the filename minus any leading directories.
 */
char const *getbasefilename(char const *name)
{
    char const *r;

    r = strrchr(name, '\\');
    return r && r[1] ? r + 1 : name;
}
