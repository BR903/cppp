#ifndef	_exptree_h_
#define	_exptree_h_

#include	"error.h"
#include	"strset.h"
#include	"clex.h"

#define	MAX_EXPTREECHILDREN	4    

typedef	struct exptree exptree;
struct exptree {
    int		exp;
    char const *begin;
    char const *end;
    int		valued;
    long	value;
    int		opid;
    char const *identifier;
    exptree    *child[MAX_EXPTREECHILDREN];
    int		childcount;
    errhandler *err;
};

extern void initexptree(exptree *t, errhandler *err);
extern void drawtree(exptree const *t);
extern void cleartree(exptree *t, int discard);

extern int valuedp(exptree const *t);
extern long getvalue(exptree const *t);
extern int setvalue(exptree *t, int valued, long value);
extern int getexplength(exptree const *t);
extern int getexperror(exptree const *t);
extern char const *getexp(exptree const *t);
extern int getexptype(exptree const *t);

extern int addchild(exptree *t, exptree *node, int pos);
extern exptree *addnewchild(exptree *t, int pos);
extern exptree *removechild(exptree *t, int pos);
extern int discardchild(exptree *t, int pos);
extern exptree *duptree(exptree const *t);

extern int markdefined(exptree *t, strset *set, int defined);
extern char const *parseconstant(exptree *t, clexer *cl, char const *input);
extern char const *parseexp(exptree *t, clexer *cl, char const *exp, int prec);
extern long evaltree(exptree *t, int *defined);
extern int unparseevaluated(exptree const *t, char *buffer);

#endif
