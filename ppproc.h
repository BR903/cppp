#ifndef	_ppproc_h_
#define	_ppproc_h_

#include	<stdio.h>
#include	"error.h"
#include	"strset.h"
#include	"clex.h"

typedef	struct ppproc {
    clexer     *cl;
    strset     *defs;
    strset     *undefs;
    int		level;
    int	       *stack;
    char       *line;
    int		linesize;
    int		copy;
    int		absorb;
    int		endline;
    errhandler *err;
} ppproc;

extern void initppproc(ppproc *ppp, strset *defs, strset *undefs,
				    errhandler *err);
extern void closeppproc(ppproc *ppp);
extern void beginfile(ppproc *ppp);
extern void endfile(ppproc *ppp);

extern int readline(ppproc *ppp, FILE *infile);
extern int writeline(ppproc *ppp, FILE *outfile);
extern void prepreprocess(ppproc *ppp, FILE *infile, FILE *outfile);

#endif
