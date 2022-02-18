/* mstr.c: Copyright (C) 2022 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "gen.h"
#include "types.h"
#include "mstr.h"

/* Specification of substring.
 */
typedef struct chspan {
    int         from;           /* index of start of substring */
    int         to;             /* first byte past substring */
} chspan;

/* A modifiable string with two representations, an editable
 * presentation form and an underlying base form.
 */
struct mstr {
    char       *str;            /* the edited, or "presentation" string */
    int         length;         /* the length of the string */
    int         allocated;      /* the size of the string buffer (and spans) */
    chspan     *spans;          /* what substring each char represents */
    char       *basestr;        /* the underlying, or "base" string */
    int         baselength;     /* the length of the base string */
    int         baseallocated;  /* the size of the base buffer */
};

/* Allocates an empty mstr object.
 */
mstr *initmstr(void)
{
    mstr *ms;

    ms = allocate(sizeof *ms);
    ms->str = NULL;
    ms->length = 0;
    ms->allocated = 0;
    ms->spans = NULL;
    ms->basestr = NULL;
    ms->baselength = 0;
    ms->baseallocated = 0;
    return ms;
}

/* Deallocates the mstr object.
 */
void freemstr(mstr *ms)
{
    deallocate(ms->str);
    deallocate(ms->spans);
    deallocate(ms->basestr);
    deallocate(ms);
}

/* Prepares an mstr for a change in size, reallocating its buffers as
 * necessary.
 */
static void grow(mstr *ms, int add)
{
    if (add > ms->allocated - ms->length - 1) {
        ms->allocated += ms->allocated > add ? ms->allocated : add;
        if (ms->allocated < 32)
            ms->allocated = 32;
        ms->str = reallocate(ms->str, ms->allocated);
        ms->spans = reallocate(ms->spans, ms->allocated * sizeof *ms->spans);
    }
    if (add > ms->baseallocated - ms->baselength - 1) {
        ms->baseallocated += ms->baseallocated > add ? ms->baseallocated : add;
        if (ms->baseallocated < 32)
            ms->baseallocated = 32;
        ms->basestr = reallocate(ms->basestr, ms->baseallocated);
    }
}

/* Returns the length of the presentation string.
 */
int getmstrlen(mstr const *ms)
{
    return ms->length;
}

/* Returns the length of the base string.
 */
int getmstrbaselen(mstr const *ms)
{
    return ms->baselength;
}

/* Returns a pointer to the presentation string buffer.
 */
char const *getmstrbuf(mstr const *ms)
{
    if (!ms->str)
        grow((mstr*)ms, 1);
    ms->str[ms->length] = '\0';
    return ms->str;
}

/* Returns a pointer to the base string buffer.
 */
char const *getmstrbase(mstr const *ms)
{
    if (!ms->basestr)
        grow((mstr*)ms, 1);
    ms->basestr[ms->baselength] = '\0';
    return ms->basestr;
}

/* Resets the mstr to an empty string.
 */
void erasemstr(mstr *ms)
{
    ms->length = 0;
    ms->baselength = 0;
}

/* Resets the mstr to the given string value.
 */
int setmstr(mstr *ms, char const *str)
{
    size_t size;
    int i;

    size = strlen(str);
    if (size >= INT_MAX)
        return 0;
    grow(ms, size + 1 - ms->length);
    memcpy(ms->str, str, size);
    memcpy(ms->basestr, str, size);
    ms->length = size;
    ms->baselength = size;
    for (i = 0 ; i < ms->length ; ++i) {
        ms->spans[i].from = i;
        ms->spans[i].to = i + 1;
    }
    return 1;
}

/* Adds a single character to the string.
 */
int appendmstr(mstr *ms, char ch)
{
    if (ms->length >= INT_MAX - 1)
        return 0;
    grow(ms, 1);
    ms->spans[ms->length].from = ms->baselength;
    ms->spans[ms->length].to = ms->baselength + 1;
    ms->str[ms->length++] = ch;
    ms->basestr[ms->baselength++] = ch;
    return 1;
}

/* Replaces part of the string with another string. The spans array is
 * used to determine how to apply the same edit to the underlying base
 * string. If the size of the string changes, the values in the spans
 * array will be updated appropriately.
 */
char const *editmstr(mstr *ms, char const *old, int oldlen,
                     char const *new, int newlen)
{
    int pos, delta;
    int basepos, baselen, basedelta;
    int i;

    pos = old - ms->str;
    delta = newlen - oldlen;
    if (delta > 0) {
        if (ms->length >= INT_MAX - delta)
            return NULL;
        grow(ms, delta);
    }
    basepos = ms->spans[pos].from;
    baselen = oldlen == 0 ? 0 : ms->spans[pos + oldlen - 1].to - basepos;
    basedelta = newlen - baselen;

    if (delta) {
        memmove(ms->str + pos + newlen, ms->str + pos + oldlen,
                ms->length + 1 - pos - oldlen);
        memmove(ms->spans + pos + newlen, ms->spans + pos + oldlen,
                (ms->length + 1 - pos - oldlen) * sizeof *ms->spans);
        ms->length += delta;
    }
    if (basedelta) {
        memmove(ms->basestr + basepos + newlen,
                ms->basestr + basepos + baselen,
                ms->baselength + 1 - basepos - baselen);
        ms->baselength += basedelta;
    }
    memcpy(ms->str + pos, new, newlen);
    memcpy(ms->basestr + basepos, new, newlen);

    for (i = 0 ; i < newlen ; ++i) {
        ms->spans[pos + i].from = basepos + i;
        ms->spans[pos + i].to = basepos + i + 1;
    }
    if (basedelta) {
        for (i += pos ; i < ms->length ; ++i) {
            ms->spans[i].from += basedelta;
            ms->spans[i].to += basedelta;
        }
    }

    return ms->str + pos;
}

/* Deletes part of the presentation string, optionally replacing it
 * with a single character, without affecting the underlying base
 * string. If new is nonzero, then the single character is mapped to
 * the deleted substring in the base string; otherwise the deleted
 * substring is left unmapped.
 */
void altermstr(mstr *ms, char const *old, int len, char new)
{
    int pos;

    pos = old - ms->str;
    if (new) {
        ms->str[pos] = new;
        ms->length -= len - 1;
        ms->spans[pos].to = ms->spans[pos + len - 1].to;
        memmove(ms->str + pos + 1, ms->str + pos + len, ms->length - pos - 1);
        memmove(ms->spans + pos + 1, ms->spans + pos + len,
                (ms->length - pos - 1) * sizeof *ms->spans);
    } else {
        ms->length -= len;
        memmove(ms->str + pos, ms->str + pos + len, ms->length - pos);
        memmove(ms->spans + pos, ms->spans + pos + len,
                (ms->length - pos) * sizeof *ms->spans);
    }
}
