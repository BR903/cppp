#include	<stdio.h>
#include	<string.h>
#include	<stdarg.h>
#include	<errno.h>
#include	"error.h"

void initerrhandler(errhandler *e)
{
    e->file = NULL;
    e->lineno = 0;
    e->err = errNone;
    e->count = 0;
}

void reseterrhandler(errhandler *e)
{
    e->err = errNone;
    e->count = 0;
}

void seterrorfile(errhandler *e, char const *file)
{
    e->file = file;
    e->lineno = 0;
    e->err = errNone;
}

void seterrorline(errhandler *e, unsigned long lineno)
{
    e->lineno = lineno;
}

void nexterrorline(errhandler *e)
{
    ++e->lineno;
}

int getlasterror(errhandler *e)
{
    return e->err;
}

int geterrormark(errhandler *e)
{
    return e->count;
}

int errorsincemark(errhandler *e, int mark)
{
    return e->count > mark;
}

void error(errhandler *e, int err, ...)
{
    va_list	args;
    int		n;

    e->err = err;
    if (err == errNone)
	return;
    ++e->count;

    if (e->file)
	if (e->lineno)
	    fprintf(stderr, "%s:%lu: ", e->file, e->lineno);
	else
	    fprintf(stderr, "%s: ", e->file);
    else
	if (e->lineno)
	    fprintf(stderr, "line %lu: ", e->lineno);
	else
	    fputs("error: ", stderr);

    va_start(args, err);
    switch (err) {
      case errLowMem:
	fputs("out of memory.", stderr);
	break;
      case errSyntax:
	fputs("preprocessor syntax error.", stderr);
	break;
      case errFileIO:
	if (errno)
	    fputs(strerror(errno), stderr);
	else
	    fputs("file I/O error.", stderr);
	break;
      case errIfsTooDeep:
	fprintf(stderr, "too many nested ifs.");
	break;
      case errDanglingElse:
	fputs("else not matched to any if.", stderr);
	break;
      case errDanglingEnd:
	fputs("endif found without any if.", stderr);
	break;
      case errOpenIf:
	n = va_arg(args, int);
	if (n == 1)
	    fputs("if not closed.", stderr);
	else
	    fprintf(stderr, "%d ifs not closed.", n);
	break;
      case errBadCharLiteral:
	fputs("bad character literal.", stderr);
	break;
      case errOpenCharLiteral:
	fputs("last character literal not closed.", stderr);
	break;
      case errOpenStringLiteral:
	fputs("last string literal not closed.", stderr);
	break;
      case errOpenComment:
	fputs("last comment not closed.", stderr);
	break;
      case errOpenParenthesis:
	fputs("unmatched left parenthesis.", stderr);
	break;
      case errEmptyIf:
	fputs("if with no identifier.", stderr);
	break;
      case errMissingOperand:
	fputs("operator with missing expression.", stderr);
	break;
      case errBadPreprocessor:
	fputs("bad preprocessor statement.", stderr);
	break;
      case errIfSyntax:
	fputs("bad syntax in #if expression.", stderr);
	break;
      case errDefinedSyntax:
	fputs("bad syntax in defined operator.", stderr);
      case errBrokenComment:
	fputs("comment spans deleted line.", stderr);
	break;
      default:
	fprintf(stderr, "number %d.", err);
	break;
    }
    va_end(arg);

    fputc('\n', stderr);
}
