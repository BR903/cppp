/* unixisms.c: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "unixisms.h"

/* File descriptor of the saved directory.
 */
static int currentdir = -1;

/* Changes the current directory.
 */
int changedir(char const *name)
{
    return chdir(name) == 0;
}

/* Opens a file descriptor on the current directory.
 */
int savedir(void)
{
    currentdir = open(".", O_RDONLY);
    return currentdir >= 0;
}

/* Uses the saved file description to change the directory back.
 */
int restoredir(void)
{
    if (fchdir(currentdir) != 0)
	return 0;
    close(currentdir);
    currentdir = -1;
    return 1;
}

/* Returns true if the filename is a directory.
 */
int fileisdir(char const *name)
{
    struct stat s;

    if (stat(name, &s))
	return 0;
    return S_ISDIR(s.st_mode);
}

/* Returns a pointer to the filename minus any leading directories.
 */
char const *getbasefilename(char const *name)
{
    char const *r;

    r = strrchr(name, '/');
    return r && r[1] ? r + 1 : name;
}
