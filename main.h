#ifndef	_main_h_
#define	_main_h_

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#define	sizearray(a)	((int)(sizeof (a) / sizeof *(a)))

#define _issym(ch)	(isalnum(ch) || (ch) == '_')

extern void *xmalloc(size_t size);
extern void *xrealloc(void *p, size_t size);

#endif
