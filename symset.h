/* symset.h: Copyright (C) 2011-2022 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#ifndef _symset_h_
#define _symset_h_

/*
 * A symset is an unordered set of symbols, which in turn are id/value
 * pairs, the ids being macro names. The strings representing the ids
 * are not copied by the symset objects; the caller retains ownership.
 */

#include "types.h"

/* Creates an empty set of symbols.
 */
extern symset *initsymset(void);

/* Deallocates the set of symbols.
 */
extern void freesymset(symset *set);

/* Adds a symbol to the set.
 */
extern void addsymboltoset(symset *set, char const *id, long value);

/* Finds a symbol in a set. id points to an identifier, typically not
 * NUL-delimited but embedded within a larger string. The return value
 * is true if a symbol with that name is a member of the set. If value
 * is not NULL, it receives the found symbol's value.
 */
extern int findsymbolinset(symset const *set, char const *id, long *value);

/* Removes a symbol from the set. The return value is false if the
 * symbol was not a member of the set.
 */
extern int removesymbolfromset(symset *set, char const *id);

#endif
