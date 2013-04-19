#ifndef	_clex_h_
#define	_clex_h_

#include	"error.h"

enum
{
    cmdNone = 0,
    cmdIf, cmdIfdef, cmdIfndef, cmdElse, cmdElif, cmdEndif,
    cmdDefine, cmdUndef, cmdInclude, cmdLine, cmdError, cmdPragma,
    cmdBad
};

typedef struct clexer {
    int		state;
    int		charquote;
    int		parenlevel;
    int		bracketlevel;
    int		bracelevel;
    int		examined;
    int		charcount;
    errhandler *err;
} clexer;

extern void initclexer(clexer *cl, errhandler *err);
extern void endstream(clexer *cl);

extern int endoflinep(clexer const *cl);
extern int whitespacep(clexer const *cl);
extern int charquotep(clexer const *cl);
extern int stringp(clexer const *cl);
extern int ccommentp(clexer const *cl);
extern int cppcommentp(clexer const *cl);
extern int preprocessorlinep(clexer const *cl);

extern int getidentifierlength(char const *input);

extern char const *examinechar(clexer *cl, char const *input);
extern char const *nextchars(clexer *cl, char const *input, int skip);
extern char const *nextline(clexer *cl, char const *input);
extern char const *restofline(clexer *cl, char const *input);
extern char const *skipwhite(clexer *cl, char const *input);
extern char const *skiptowhite(clexer *cl, char const *input);
extern char const *getpreprocessorcmd(clexer *cl, char const *input, int *cmd);
extern char const *getidentifier(clexer *cl, char const *input, char *buffer);

#define	nextchar(cl, input)	(nextchars(cl, input, 1))

#endif

