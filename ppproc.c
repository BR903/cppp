#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	"main.h"
#include	"error.h"
#include	"strset.h"
#include	"exptree.h"
#include	"ppproc.h"

static int const maxstack = 1024;

enum
{
    fIf = 0x0001, fElse = 0x0002, fElif = 0x0004, fOurs = 0x0010,
    fCopy = 0x0020, fIfModify = 0x0040, fElseModify = 0x0080
};

enum
{
    statError = 0, statDefined, statUndefined, statPartDefined, statUnaffected
}; 

static char const *seqif(ppproc *ppp, char *ifexp, int *status)
{
    exptree	tree;
    char const *ret;
    char       *s;
    int		defined, n;

    initexptree(&tree, ppp->err);
    *status = statUnaffected;

    n = geterrormark(ppp->err);
    ret = parseexp(&tree, ppp->cl, ifexp, 0);
    if (errorsincemark(ppp->err, n)) {
	*status = statError;
	goto quit;
    } else if (!endoflinep(ppp->cl)) {
	error(ppp->err, errIfSyntax);
	*status = statError;
	goto quit;
    }

    n = 0;
    if (ppp->defs)
	n += markdefined(&tree, ppp->defs, TRUE);
    if (ppp->undefs)
	n += markdefined(&tree, ppp->undefs, FALSE);
    if (n) {
	*status = evaltree(&tree, &defined) ? statDefined : statUndefined;
	if (!defined) {
	    *status = statPartDefined;
	    s = xmalloc(strlen(ifexp) + 1);
	    n = unparseevaluated(&tree, s);
	    strcpy(s + n, ifexp + getexplength(&tree));
	    strcpy(ifexp, s);
	    free(s);
	}
    }

  quit:
    cleartree(&tree, TRUE);
    return ret;
}

static void seq(ppproc *ppp)
{
    char const *in;
    char const *cmd;
    int		incomment, status;
    int		id, size, n;

    incomment = ccommentp(ppp->cl);
    ppp->absorb = FALSE;
    in = examinechar(ppp->cl, ppp->line);
    while (!preprocessorlinep(ppp->cl)) {
	if (endoflinep(ppp->cl))
	    return;
	in = nextchar(ppp->cl, in);
    }

    cmd = skipwhite(ppp->cl, nextchar(ppp->cl, in));
    in = getpreprocessorcmd(ppp->cl, cmd, &id);

    switch (id) {
      case cmdIfdef:
      case cmdIfndef:
	if (ppp->level >= maxstack - 1) {
	    error(ppp->err, errIfsTooDeep);
	    break;
	}
	++ppp->level;
	ppp->stack[ppp->level] = ppp->copy ? fCopy : 0;
	if (!ppp->copy) {
	    in = restofline(ppp->cl, in);
	    break;
	}
	size = getidentifierlength(in);
	if (!size) {
	    error(ppp->err, errEmptyIf);
	    break;
	}
	if (ppp->defs && findsymbolinset(ppp->defs, in))
	    n = statDefined;
	else if (ppp->undefs && findsymbolinset(ppp->undefs, in))
	    n = statUndefined;
	else
	    n = statUnaffected;
	in = skipwhite(ppp->cl, nextchars(ppp->cl, in, size));
	if (!endoflinep(ppp->cl)) {
	    error(ppp->err, errSyntax);
	    break;
	}
	if (n != statUnaffected) {
	    ppp->absorb = TRUE;
	    ppp->stack[ppp->level] |= fOurs;
	    ppp->copy = n == (id == cmdIfdef ? statDefined : statUndefined);
	}
	break;

      case cmdIf:
	if (ppp->level >= maxstack - 1) {
	    error(ppp->err, errIfsTooDeep);
	    break;
	}
	++ppp->level;
	ppp->stack[ppp->level] = fIf | (ppp->copy ? fCopy : 0);
	if (!ppp->copy) {
	    in = restofline(ppp->cl, in);
	    break;
	}
	in = seqif(ppp, (char*)in, &status);
	if (status == statError)
	    break;
	else if (!endoflinep(ppp->cl)) {
	    error(ppp->err, errIfSyntax);
	    break;
	}
	if (status == statDefined || status == statUndefined) {
	    ppp->absorb = TRUE;
	    ppp->stack[ppp->level] |= fOurs;
	    ppp->copy = status == statDefined;
	}
	break;

      case cmdElse:
	if (ppp->level < 0 || (ppp->stack[ppp->level] & fElse)) {
	    error(ppp->err, errDanglingElse);
	    break;
	}
	ppp->stack[ppp->level] |= fElse;
	if (!endoflinep(ppp->cl)) {
	    error(ppp->err, errSyntax);
	    break;
	}
	if (ppp->stack[ppp->level] & fOurs) {
	    ppp->copy = !ppp->copy;
	    ppp->absorb = TRUE;
	    n = ppp->level;
	    while (ppp->stack[n] & fElif) {
		if (ppp->stack[n] & fElseModify) {
		    ppp->absorb = TRUE;
		    break;
		}
		--n;
		if (!(ppp->stack[n] & fOurs))
		    ppp->absorb = FALSE;
	    }
	}
	break;

      case cmdElif:
	if (ppp->level < 0 || !(ppp->stack[ppp->level] & fIf)
			   || (ppp->stack[ppp->level] & fElse)) {
	    error(ppp->err, errDanglingElse);
	    break;
	} else if (ppp->level >= maxstack - 1) {
	    error(ppp->err, errIfsTooDeep);
	    break;
	}
	ppp->stack[ppp->level] |= fElse;
	if (ppp->stack[ppp->level] & fOurs)
	    ppp->copy = !ppp->copy;
	++ppp->level;
	ppp->stack[ppp->level] = fIf | fElif | (ppp->copy ? fCopy : 0);
	if (!ppp->copy) {
	    in = restofline(ppp->cl, in);
	    break;
	}
	in = seqif(ppp, (char*)in, &status);
	if (status == statError)
	    break;
	else if (!endoflinep(ppp->cl)) {
	    error(ppp->err, errIfSyntax);
	    break;
	}
	if (status == statUndefined) {
	    ppp->copy = FALSE;
	    ppp->absorb = TRUE;
	    ppp->stack[ppp->level] |= fOurs;
	} else if (status == statDefined) {
	    ppp->absorb = TRUE;
	    n = ppp->level;
	    while (ppp->stack[n] & fElif) {
		--n;
		if (!(ppp->stack[n] & fOurs)) {
		    strcpy((char*)cmd, "else");
		    ppp->stack[ppp->level] |= fElseModify;
		    ppp->absorb = FALSE;
		    break;
		}
	    }
	    ppp->stack[ppp->level] |= fOurs;
	} else {
	    n = ppp->level;
	    while (ppp->stack[n] & fElif) {
		--n;
		if (!(ppp->stack[n] & fOurs)) {
		    n = -1;
		    break;
		}
	    }
	    if (n >= 0) {
		memmove((char*)cmd, cmd + 2, strlen(cmd + 2) + 1);
		ppp->stack[ppp->level] |= fIfModify;
	    }
	}
	break;

      case cmdEndif:
	if (ppp->level < 0) {
	    error(ppp->err, errDanglingEnd);
	    break;
	}
	if (!endoflinep(ppp->cl)) {
	    error(ppp->err, errSyntax);
	    break;
	}
	ppp->absorb = TRUE;
	for ( ; ppp->stack[ppp->level] & fElif ; --ppp->level) {
	    if (ppp->stack[ppp->level] & (fIfModify | fElseModify))
		ppp->absorb = FALSE;
	}
	if (ppp->absorb)
	    ppp->absorb = ppp->stack[ppp->level] & fOurs;
	ppp->copy = ppp->stack[ppp->level] & fCopy;
	--ppp->level;
	break;

      case cmdBad:
	break;

      default:
	in = restofline(ppp->cl, in);
	break;
    }

    if (ppp->absorb && incomment != ccommentp(ppp->cl))
	error(ppp->err, errBrokenComment);
}


void initppproc(ppproc *ppp, strset *defs, strset *undefs, errhandler *err)
{
    ppp->cl = xmalloc(sizeof *ppp->cl);
    ppp->defs = defs;
    ppp->undefs = undefs;
    ppp->stack = xmalloc(maxstack * sizeof *ppp->stack);
    ppp->line = NULL;
    ppp->linesize = 0;
    ppp->err = err;
}

void closeppproc(ppproc *ppp)
{
    free(ppp->cl);
    ppp->cl = NULL;
    free(ppp->stack);
    ppp->stack = NULL;
    free(ppp->line);
    ppp->line = NULL;
}


void beginfile(ppproc *ppp)
{
    initclexer(ppp->cl, ppp->err);
    free(ppp->line);
    ppp->line = NULL;
    ppp->linesize = 0;
    ppp->level = -1;
    ppp->copy = TRUE;
    ppp->absorb = FALSE;
}

void endfile(ppproc *ppp)
{
    endstream(ppp->cl);
    if (ppp->level != -1)
	error(ppp->err, errOpenIf, ppp->level + 1);

    free(ppp->line);
    ppp->line = NULL;
    ppp->linesize = 0;
}


int readline(ppproc *ppp, FILE *infile)
{
    fpos_t	pos;
    int		size;
    int		prev = EOF;
    int		ch;

    fgetpos(infile, &pos);
    ch = fgetc(infile);
    for (size = 0 ; ch != EOF ; ++size) {
	if (ch == '\n' && prev != '\\')
	    break;
	prev = ch;
	ch = fgetc(infile);
    }
    if (!size && ch == EOF) {
	ppp->line = NULL;
	return 0;
    }

    fsetpos(infile, &pos);
    if (size >= ppp->linesize) {
	ppp->linesize = size + 1;
	ppp->line = xrealloc(ppp->line, ppp->linesize);
    }
    if (size) {
	if (fread(ppp->line, size, 1, infile) != 1) {
	    error(ppp->err, errFileIO);
	    return -1;
	}
    }
    ppp->endline = ch != EOF;
    if (ppp->endline)
	fgetc(infile);
    ppp->line[size] = '\0';
    seq(ppp);

    nextline(ppp->cl, NULL);
    return size;
}

int writeline(ppproc *ppp, FILE* outfile)
{
    int	size;

    if (!ppp->line)
	return 0;
    if (!ppp->copy || ppp->absorb)
	return 0;

    size = strlen(ppp->line);
    if (size) {
	if (fwrite(ppp->line, size, 1, outfile) != 1) {
	    seterrorfile(ppp->err, NULL);
	    error(ppp->err, errFileIO);
	    return -1;
	}
    }
    if (ppp->endline)
	fputc('\n', outfile);
    ppp->endline = FALSE;
    return size;
}

void prepreprocess(ppproc *ppp, FILE *infile, FILE *outfile)
{
    beginfile(ppp);
    seterrorline(ppp->err, 1);
    while (!feof(infile) && !ferror(infile)) {
	readline(ppp, infile);
	writeline(ppp, outfile);
	nexterrorline(ppp->err);
    }
    seterrorline(ppp->err, 0);
    endfile(ppp);
}
