/* mstr.h: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef _mstr_h_
#define _mstr_h_

/*
 * An mstr is an modifiable string, which manages two separate
 * representations, or forms. There is an editable "presentation" form
 * and an underlying "base" form. Most modifications apply to both
 * forms of the string, but it is also possible to make alterations
 * that apply only to the presentation form. After such alterations,
 * further modifications will be still be applied correctly to the
 * underlying base form. Thus, parts of a string can be "hidden" by
 * altering only the presentation form, and then restored after
 * parsing and editing are done.
 */

#include "types.h"

/* Creates a new mstr object, containing an empty string.
 */
extern mstr *initmstr(void);

/* Deallocates the mstr.
 */
extern void freemstr(mstr *ms);

/* Resets the mstr to an empty string.
 */
extern void erasemstr(mstr *ms);

/* Returns the length of the string.
 */
extern int getmstrlen(mstr const *ms);

/* Returns the length of the underlying base string.
 */
extern int getmstrbaselen(mstr const *ms);

/* Returns a buffer containing the string's presentation form. This
 * pointer will remain valid as long as no modifications are made to
 * mstr.
 */
extern char const *getmstrbuf(mstr const *ms);

/* Returns a buffer containing the string's base form. This pointer
 * will remain valid as long as no modifications are made to mstr.
 */
extern char const *getmstrbase(mstr const *ms);

/* Resets an mstr to a new string. Returns false if the provided
 * string is too large.
 */
extern int setmstr(mstr *ms, char const *str);

/* Modifies the mstr's string by appending a single character. Returns
 * false if the string is already at maximum length.
 */
extern int appendmstr(mstr *ms, char ch);

/* Modifies the mstr's string by replacing one substring with another.
 * The old pointer must be a pointer into the buffer previously
 * returned by getmstrbuf(). Either oldlen or newlen can be zero. The
 * return value is the updated value for old. (The caller must not use
 * pointers based on the prior call to getmstrbuf() after this
 * function returns.) If making the edit would cause the string to be
 * too long, NULL is returned and the object remains unchanged.
 */
extern char const *editmstr(mstr *ms, char const *old, int oldlen,
                            char const *new, int newlen);

/* Alters the mstr's presentation form only by "hiding" a substring,
 * optionally replacing it with a single character. The underlying
 * base form is unchanged. old must point into the buffer previously
 * returned by getmstrbuf(). The value of len must be positive. If new
 * is nonzero, then the substring is replaced with that character;
 * otherwise the substring is simply removed. (Existing pointers are
 * not invalidated by this function, since it never increases the size
 * of the presentation string.)
 */
extern void altermstr(mstr *ms, char const *old, int len, char new);

#endif
