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

struct ppproc;
struct symset;

/* Creates a ppproc object initialized with pre-defined sets of defined
 * and undefined symbols.
 */
extern struct ppproc *initppproc(struct symset *defs, struct symset *undefs);

/* Deallocates the ppproc object.
 */
extern void freeppproc(struct ppproc *ppp);

/* Partially preprocesses infile's contents to outfile.
 */
extern void partialpreprocess(struct ppproc *ppp, void *infile, void *outfile);

#endif
