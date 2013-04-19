#include	<string.h>
#include	<ctype.h>
#include	"main.h"
#include	"error.h"
#include	"clex.h"

#define	_end	(-256)

enum
{
    fEndOfLine = 0x0001, fInCharQuote = 0x0002, fInString = 0x0004,
    fInComment = 0x0010, fInComment1Line = 0x0020,
    fInAnyComment = 0x0030, fLeavingCharQuote = 0x0040,
    fLeavingString = 0x0080, fLeavingComment = 0x0100,
    fSeen1st = 0x0200, fPreprocess = 0x0400,
    fWhitespace = 0x0800
};


static char const *readwhack(clexer *cl, char const *input)
{
    char const *in = input;
    int		n;

    if (*in != '\\')
	return in;

    ++in;
    ++cl->charcount;
    if (*in >= '0' && *in <= '7') {
	cl->examined = 0;
	for (n = 0 ; n < 3 ; ++n) {
	    if (in[n] >= '0' && in[n] <= '7')
		cl->examined = (cl->examined << 3) + in[n] - '0';
	    else
		break;
	}
	in += n - 1;
	cl->charcount += n - 1;
    } else if (*in == 'x') {
	cl->examined = 0;
	for (n = 1 ; n < 3 ; ++n) {
	    if (in[n] >= '0' && in[n] <= '9')
		cl->examined = (cl->examined << 4) + in[n] - '0';
	    else if (in[n] >= 'A' && in[n] <= 'F')
		cl->examined = (cl->examined << 4) + in[n] - 'A';
	    else if (in[n] >= 'a' && in[n] <= 'f')
		cl->examined = (cl->examined << 4) + in[n] - 'a';
	    else
		break;
	}
	in += n - 1;
	cl->charcount += n - 1;
    } else {
	switch (*in) {
	  case 'a':	cl->examined = '\a';	break;
	  case 'b':	cl->examined = '\b';	break;
	  case 'f':	cl->examined = '\f';	break;
	  case 'n':	cl->examined = '\n';	break;
	  case 'r':	cl->examined = '\r';	break;
	  case 't':	cl->examined = '\t';	break;
	  case 'v':	cl->examined = '\v';	break;
	  default:	cl->examined = *in;	break;
	}
    }

    cl->examined |= '\\' << 8;
    return in;
}


void initclexer(clexer *cl, errhandler *err)
{
    cl->err = err;
    cl->state = 0;
    cl->charquote = 0;
    cl->parenlevel = 0;
    cl->bracketlevel = 0;
    cl->bracelevel = 0;
    cl->examined = _end;
    cl->charcount = 0;
}

void endstream(clexer *cl)
{
    if (!cl->err) {
	if (cl->state & fInCharQuote)
	    error(cl->err, errOpenCharLiteral);
	else if (cl->state & fInString)
	    error(cl->err, errOpenStringLiteral);
	else if (cl->state & fInComment)
	    error(cl->err, errOpenComment);
    }
    cl->state = 0;
    cl->charcount = 0;
    cl->examined = _end;
}


int endoflinep(clexer const *cl)	{ return cl->state & fEndOfLine; }
int whitespacep(clexer const *cl)	{ return cl->state & fWhitespace; }
int charquotep(clexer const *cl)	{ return cl->state & fInCharQuote; }
int stringp(clexer const *cl)		{ return cl->state & fInString; }
int ccommentp(clexer const *cl)		{ return cl->state & fInComment; }
int cppcommentp(clexer const *cl)	{ return cl->state & fInComment1Line; }
int preprocessorlinep(clexer const *cl)	{ return cl->state & fPreprocess; }

int getidentifierlength(char const *input)
{
    char const *in;

    for (in = input ; _issym(*in) ; ++in) ;
    return (int)(in - input);
}


char const *examinechar(clexer *cl, char const *input)
{
    char const *in = input;

    if (cl->state & fLeavingComment)
	cl->state &= ~(fInComment | fLeavingComment);
    else if (cl->state & fLeavingString)
	cl->state &= ~(fInString | fLeavingString);
    else if (cl->state & fLeavingCharQuote) {
	cl->state &= ~(fInCharQuote | fLeavingCharQuote);
	cl->charquote = 0;
    }
    if (cl->state & fEndOfLine) {
	cl->charcount = 0;
	return in;
    }
    cl->charcount = 1;
    while (in[0] == '\\' && in[1] == '\n') {
	in += 2;
	cl->charcount += 2;
    }
    if (!*in || *in == '\n') {
	cl->state |= fEndOfLine;
	cl->state &= ~(fWhitespace | fInComment1Line | fPreprocess);
	return input;
    }

    if (cl->state & (fInCharQuote | fInString))
	cl->state &= ~fWhitespace;
    else if ((cl->state & fInAnyComment) || isspace(*in))
	cl->state |= fWhitespace;
    else
	cl->state &= ~fWhitespace;
    cl->examined = *in;
    if (cl->state & fInComment1Line) {
	/* do nothing */
    } else if (cl->state & fInComment) {
	if (in[0] == '*' && in[1] == '/') {
	    ++cl->charcount;
	    cl->state |= fLeavingComment;
	    cl->examined = '*' * 256 + '/';
	}
    } else if (cl->state & fInCharQuote) {
	if (*in == '\\') {
	    readwhack(cl, in);
	    ++cl->charquote;
	} else if (*in == '\'') {
	    if (!cl->charquote) {
		error(cl->err, errBadCharLiteral);
		cl->state |= fEndOfLine;
		return input;
	    }
	    cl->state |= fLeavingCharQuote;
	} else
	    ++cl->charquote;
	if (cl->charquote > 2) {			/* optional */
	    error(cl->err, errBadCharLiteral);
	    cl->state |= fEndOfLine;
	    return input;
	}
    } else if (cl->state & fInString) {
	if (*in == '\\')
	    readwhack(cl, in);
	else if (*in == '"')
	    cl->state |= fLeavingString;
    } else {
	if (*in == '/') {
	    if (in[1] == '/') {				/* optional */
		++cl->charcount;
		cl->state |= fInComment1Line | fWhitespace;
		cl->examined = '/' * 256 + '/';
	    } else if (in[1] == '*') {
		++cl->charcount;
		cl->state |= fInComment | fWhitespace;
		cl->examined = '/' * 256 + '*';
	    }
	}
	if (!(cl->state & (fWhitespace | fSeen1st))) {
	    cl->state |= fSeen1st;
	    if (*in == '#')
		cl->state |= fPreprocess;
	}
	if (!(cl->state & fWhitespace)) {
	    if (*in == '\'') {
		cl->state |= fInCharQuote;
		cl->charquote = 0;
	    } else if (*in == '"')
		cl->state |= fInString;
	    else if (*in == 'L') {
		if (in[1] == '\'') {
		    ++cl->charcount;
		    cl->state |= fInCharQuote;
		    cl->examined = 'L' * 256 + '\'';
		    cl->charquote = 0;
		} else if (in[1] == '"') {
		    ++cl->charcount;
		    cl->state |= fInString;
		    cl->examined = 'L' * 256 + '"';
		}
	    } else if (*in == '(')
		++cl->parenlevel;
	    else if (*in == ')')
		--cl->parenlevel;
	    else if (*in == '[')
		++cl->bracketlevel;
	    else if (*in == ']')
		--cl->bracketlevel;
	    else if (*in == '{')
		++cl->bracelevel;
	    else if (*in == '}')
		--cl->bracelevel;
	}
    }
    return input;
}

char const *nextchars(clexer *cl, char const *input, int skip)
{
    char const *in;
    int		n;

    in = input;
    for (n = skip ; n && !endoflinep(cl) ; --n)
	in = examinechar(cl, in + cl->charcount);
    return in;
}

char const *skipwhite(clexer *cl, char const *input)
{
    char const *in = input;

    for ( ; whitespacep(cl) ; in = nextchar(cl, in)) ;
    return in;
}

char const *skiptowhite(clexer *cl, char const *input)
{
    char const *in = input;

    for ( ; !whitespacep(cl) && !endoflinep(cl) ; in = nextchar(cl, in)) ;
    return in;
}

char const *restofline(clexer *cl, char const *input)
{
    char const *in = input;

    for ( ; !endoflinep(cl) ; in = nextchar(cl, in)) ;
    return in;
}

char const *getpreprocessorcmd(clexer *cl, char const *line, int *cmdid)
{
    static struct { char const *cmd; int id; } const
	cmdlist[] = {
	    { "define", cmdDefine },	{ "elif", cmdElif },
	    { "else", cmdElse },	{ "endif", cmdEndif },
	    { "error", cmdError },	{ "if", cmdIf },
	    { "ifdef", cmdIfdef },	{ "ifndef", cmdIfndef },
	    { "include", cmdInclude },	{ "line", cmdLine },
	    { "pragma", cmdPragma },	{ "undef", cmdUndef }
	};

    char const *begin, *end;
    size_t	size;
    int		n;

    if (!preprocessorlinep(cl)) {
	*cmdid = cmdError;
	return line;
    }
    begin = skipwhite(cl, line);
    if (endoflinep(cl)) {
	*cmdid = cmdNone;
	return begin;
    }

    end = skiptowhite(cl, begin);
    *cmdid = cmdBad;
    size = end - begin;
    for (n = 0 ; n < sizearray(cmdlist) ; ++n) {
	if (size == strlen(cmdlist[n].cmd)
			&& !memcmp(cmdlist[n].cmd, begin, size)) {
	    *cmdid = cmdlist[n].id;
	    break;
	}
    }

    return skipwhite(cl, end);
}

char const *getidentifier(clexer *cl, char const *input, char* buffer)
{
    char const *in;
    int		n;

    for (in = input ; _issym(*in) ; in = nextchar(cl, in)) ;
    n = (int)(in - input);
    if (n)
	memcpy(buffer, input, n);
    buffer[n] = '\0';
    return skipwhite(cl, in);
}

char const *nextline(clexer *cl, char const *input)
{
    cl->state &= ~(fEndOfLine | fSeen1st | fInComment1Line | fPreprocess);
    cl->examined = _end;
    cl->charcount = 0;
    return input ? examinechar(cl, input) : NULL;
}
