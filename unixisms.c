/* unixisms.c: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "unixisms.h"

/* Changes the current directory.
 */
int changedir(char const *name)
{
    return chdir(name) == 0;
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

/* The fchdir() function makes savedir() and restoredir() trivial to
 * code, but sadly it isn't universal. To maximize portability, a
 * fallback version of these functions is provided.
 */
#if _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200809L || _BSD_SOURCE

/* File descriptor of the saved directory.
 */
static int currentdir = -1;

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
    return fchdir(currentdir) == 0;
}

/* Closes the saved directory.
 */
void unsavedir(void)
{
    close(currentdir);
    currentdir = -1;
}

#else

/* String buffer containing the saved directory.
 */
static char *currentdir = NULL;
static int currentdiralloc = 0;

/* Saves the path to the current directory.
 */
int savedir(void)
{
    if (!currentdir) {
        currentdiralloc = 256;
        currentdir = malloc(currentdiralloc);
        if (!currentdir)
            return 0;
    }
    while (!getcwd(currentdir, currentdiralloc)) {
        if (errno != ERANGE)
            return 0;
        currentdiralloc *= 2;
        currentdir = realloc(currentdir, currentdiralloc);
        if (!currentdir)
            return 0;
    }
    return 1;
}

/* Changes back to the saved directory.
 */
int restoredir(void)
{
    return changedir(currentdir);
}

/* Frees the remembered directory.
 */
void unsavedir(void)
{
    free(currentdir);
    currentdir = NULL;
    currentdiralloc = 0;
}

#endif
