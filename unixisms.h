/* unixisms.h: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef	_unixisms_h_
#define	_unixisms_h_

/*
 * Basic functionality not provided by the standard C library. The
 * implementation of these functions is platform-dependent.
 */

/* Change the current directory to the given pathname.
 */
extern int changedir(char const *name);

/* Remember the current directory.
 */
extern int savedir(void);

/* Change back to the directory that was remembered by savedir(). This
 * function must be called before savedir() can be called again.
 */
extern int restoredir(void);

/* Return true if the given pathname is a directory.
 */
extern int fileisdir(char const *name);

/* Return a pointer to the base filename part of the given pathname,
 * after any directories.
 */
extern char const *getbasefilename(char const *name);

#endif
