/* ppproc.h: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef	_ppproc_h_
#define	_ppproc_h_

/*
 * The ppproc object does the actual work of identifying preprocessor
 * statements affected by the user's requested definitions and
 * undefintions, and determining what the resulting output needs to
 * contain.
 */

#include <stdio.h>
#include "types.h"

/* Creates a ppproc object initialized with pre-defined sets of defined
 * and undefined symbols.
 */
extern ppproc *initppproc(symset const *defs, symset const *undefs);

/* Deallocates the ppproc object.
 */
extern void freeppproc(ppproc *ppp);

/* Partially preprocesses infile's contents to outfile.
 */
extern void partialpreprocess(ppproc *ppp, FILE *infile, FILE *outfile);

#endif
