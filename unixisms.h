/* unixisms.h: Copyright (C) 2011-2022 by
 * Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#ifndef _unixisms_h_
#define _unixisms_h_

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
 * function may be called repeatedly.
 */
extern int restoredir(void);

/* Forget the directory remembered by savedir().
 */
extern void unsavedir(void);

/* Return true if the given pathname is a directory.
 */
extern int fileisdir(char const *name);

/* Return a pointer to the base filename part of the given pathname,
 * after any directories.
 */
extern char const *getbasefilename(char const *name);

#endif
