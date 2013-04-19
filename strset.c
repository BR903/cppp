#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	"main.h"
#include	"strset.h"

static const size_t strsetchunk = 1024;

void initstrset(strset *set)
{
    set->start = NULL;
    set->end = NULL;
    set->allocated = 0;
}

void copystrset(strset *dest, strset const *src)
{
    size_t n;

    if (!src || !src->allocated || !src->start) {
	initstrset(dest);
	return;
    }
    dest->start = xmalloc(src->allocated);
    dest->allocated = src->allocated;
    n = src->end - src->start;
    dest->end = dest->start + n;
    memcpy(dest->start, src->start, n);
}

void freestrset(strset *set)
{
    free(set->start);
    initstrset(set);
}


static int symcmp(char const *r, char const *s)
{
    for (;;) {
	if (!_issym(*r))
	    return !_issym(*s) ? 0 : -1;
	else if (!_issym(*s))
	    return +1;
	else if (*r != *s)
	    return (int)*r - (int)*s;
	++r;
	++s;
    }
}

int findstringinset(strset *set, char const *string)
{
    char const *el;

    if (!string || !*string)
	return FALSE;
    for (el = set->start ; el < set->end ; el += strlen(el) + 1)
	if (!strcmp(el, string))
	    return TRUE;
    return FALSE;
}

int findsymbolinset(strset *set, char const *string)
{
    char const *el;

    if (!string || !*string)
	return FALSE;
    for (el = set->start ; el < set->end ; el += strlen(el) + 1)
	if (!symcmp(el, string))
	    return TRUE;
    return FALSE;
}

int addstringtoset(strset *set, char const *string)
{
    char       *endpos;
    size_t	size, newsize;

    if (!string || !*string)
	return FALSE;
    if (findstringinset(set, string))
	return TRUE;

    size = strlen(string) + 1;
    endpos = set->end;
    set->end += size;
    newsize = set->end - set->start;
    if (newsize > set->allocated) {
	set->allocated += strsetchunk;
	set->start = xrealloc(set->start, set->allocated);
	set->end = set->start + newsize;
	endpos = set->end - size;
    }

    memcpy(endpos, string, size);
    return TRUE;
}

int delstringfromset(strset *set, char const *string)
{
    char       *el;
    size_t	size;
    int		found;

    if (!string || !*string)
	return FALSE;

    found = FALSE;
    if (string >= set->start && string < set->end) {
	el = (char*)string;
	size = strlen(el) + 1;
	found = TRUE;
    } else {
	for (el = set->start ; el < set->end ; el += size) {
	    size = strlen(el) + 1;
	    if (!memcmp(el, string, size)) {
		found = TRUE;
		break;
	    }
	}
    }

    if (found) {
	set->end -= size;
	memmove(el, el + size, set->end - el);
    }
    return found;
}
