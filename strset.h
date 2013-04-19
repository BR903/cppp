#ifndef	_strset_h_
#define	_strset_h_

typedef	struct strset {
    size_t	allocated;
    char       *start;
    char       *end;
} strset;

extern void initstrset(strset *set);
extern void copystrset(strset *dest, strset const *src);
extern void freestrset(strset *set);

extern int addstringtoset(strset *set, char const *string);
extern int delstringfromset(strset *set, char const *string);
extern int findstringinset(strset *set, char const *string);
extern int findsymbolinset(strset *set, char const *string);

#endif
