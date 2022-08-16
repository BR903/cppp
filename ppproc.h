/* ppproc.h: Copyright (C) 2011-2022 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#ifndef _ppproc_h_
#define _ppproc_h_

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

/* Enable and disable trigraph handling in the preprocessor. Trigraph
 * support is disabled by default.
 */
extern void enabletrigraphs(int flag);

/* Partially preprocesses infile's contents to outfile.
 */
extern void partialpreprocess(ppproc *ppp, FILE *infile, FILE *outfile);

#endif
