#ifndef _error_h_
#define _error_h_

enum errors
{
    errNone = 0,
    errLowMem, errSyntax, errFileIO,
    errBadCharLiteral, errOpenCharLiteral, errOpenStringLiteral,
    errOpenComment, errBrokenComment,
    errDanglingElse, errDanglingEnd, errOpenIf,
    errIfsTooDeep, errOpenParenthesis, errMissingOperand,
    errEmptyIf, errIfSyntax, errDefinedSyntax, errBadPreprocessor,
    errCount
};

typedef	struct errhandler {
    char const	       *file;
    unsigned long	lineno;
    int			count;
    int			err;
} errhandler;

extern void initerrhandler(errhandler *e);
extern void reseterrhandler(errhandler *e);
extern void seterrorfile(errhandler *e, char const *file);
extern void seterrorline(errhandler *e, unsigned long lineno);
extern void nexterrorline(errhandler *e);
extern int getlasterror(errhandler *e);
extern int geterrormark(errhandler *e);
extern int errorsincemark(errhandler *e, int mark);
extern void error(errhandler *e, int err, ...);

#endif
