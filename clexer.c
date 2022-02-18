/* clexer.c: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <string.h>
#include <ctype.h>
#include "gen.h"
#include "types.h"
#include "error.h"
#include "clexer.h"

/* Flag values indicating the lexer's current state.
 */
#define	F_InCharQuote		0x0001
#define	F_LeavingCharQuote	0x0002
#define	F_InString		0x0004
#define	F_LeavingString		0x0008
#define	F_InComment		0x0010
#define	F_In99Comment		0x0020
#define	F_InAnyComment		0x0030
#define	F_LeavingComment	0x0040
#define F_LongChar		0x0080
#define	F_Whitespace		0x0100
#define	F_EndOfLine		0x0200
#define	F_Seen1st		0x0400
#define	F_Preprocess		0x0800

/* The values that comprise the state of the lexer as it reads the
 * input.
 */
struct clexer {
    int		state;		/* the current set of state flags */
    int		charquote;	/* count of characters inside single quotes */
    int		parenlevel;	/* nesting level of parentheses */
    int		charcount;	/* actual size of current character token */
};

/* The list of preprocess statements that the program knows about,
 * and their string representations.
 */
struct ppstatement {
    char const *statement;
    enum ppcmd	id;
};

static struct ppstatement const ppstatements[] = {
    { "define",   cmdDefine   },
    { "elif",     cmdElif     },
    { "elifdef",  cmdElifdef  },
    { "elifndef", cmdElifndef },
    { "else",     cmdElse     },
    { "endif",    cmdEndif    },
    { "if",       cmdIf       },
    { "ifdef",    cmdIfdef    },
    { "ifndef",   cmdIfndef   },
    { "undef",    cmdUndef    }
};


/* Creates a C lexer object.
 */
clexer *initclexer(void)
{
    clexer *cl;

    cl = allocate(sizeof *cl);
    cl->state = 0;
    cl->charquote = 0;
    cl->parenlevel = 0;
    cl->charcount = 0;
    return cl;
}

/* Deletes a C lexer object.
 */
void freeclexer(clexer *cl)
{
    deallocate(cl);
}

/* Boolean functions that report on various aspects of the lexer's
 * current state.
 */
int endoflinep(clexer const *cl)   { return cl->state & F_EndOfLine; }
int whitespacep(clexer const *cl)  { return cl->state & F_Whitespace; }
int charquotep(clexer const *cl)   { return cl->state & F_InCharQuote; }
int ccommentp(clexer const *cl)    { return cl->state & F_InComment; }
int preproclinep(clexer const *cl) { return cl->state & F_Preprocess; }

/* Returns the current parenthesis nesting level.
 */
int getparenlevel(clexer const *cl) { return cl->parenlevel; }

/* Returns the length of the identifier at the given position.
 */
int getidentifierlength(char const *input)
{
    int n;

    for (n = 0 ; _issym(input[n]) ; ++n) ;
    return n;
}

/* Reads past an escape sequence.
 */
static char const *readwhack(clexer *cl, char const *input)
{
    int n;

    if (*input != '\\')
	return input;

    ++input;
    ++cl->charcount;
    if (*input >= '0' && *input <= '7') {
	for (n = 1 ; n < 3 ; ++n)
	    if (input[n] < '0' || input[n] > '7')
		break;
	input += n - 1;
	cl->charcount += n - 1;
    } else if (*input == 'x') {
	for (n = 1 ; n < 3 ; ++n)
	    if (!isxdigit(input[n]))
		break;
	if (n != 3)
	    error(errBadCharLiteral);
	input += n - 1;
	cl->charcount += n - 1;
    } else {
	switch (*input) {
	  case 'a':	case '"':
	  case 'b':	case '\'':
	  case 'f':	case '\?':
	  case 'n':	case '\\':
	  case 'r':
	  case 't':
	  case 'v':
	    break;
	  default:
	    error(errBadCharLiteral);
	    break;
	}
    }

    return input;
}

/* Treats the byte (or bytes) pointed to by input as the next
 * characters in the input stream, and updates the lexer state
 * accordingly. String constants, character constants, and comments
 * are detected and tracked so that preprocessor statements can be
 * correctly identified. The return value is always the same as the
 * second parameter.
 */
static char const *examinechar(clexer *cl, char const *input)
{
    char const *in;

    in = input;
    if (cl->state & F_LeavingComment) {
	cl->state &= ~(F_InComment | F_LeavingComment);
    } else if (cl->state & F_LeavingString) {
	cl->state &= ~(F_InString | F_LeavingString);
    } else if (cl->state & F_LeavingCharQuote) {
	cl->state &= ~(F_InCharQuote | F_LeavingCharQuote);
	cl->charquote = 0;
    }
    if (cl->state & F_EndOfLine) {
	cl->charcount = 0;
	return input;
    }
    cl->charcount = 1;
    if (!*in || *in == '\n') {
	cl->state |= F_EndOfLine;
	cl->state &= ~(F_Whitespace | F_In99Comment | F_Preprocess);
	return input;
    }

    if (cl->state & (F_InCharQuote | F_InString))
	cl->state &= ~F_Whitespace;
    else if ((cl->state & F_InAnyComment) || isspace(*in))
	cl->state |= F_Whitespace;
    else
	cl->state &= ~F_Whitespace;
    if (cl->state & F_In99Comment) {
	/* do nothing */
    } else if (cl->state & F_InComment) {
	if (in[0] == '*' && in[1] == '/') {
	    ++cl->charcount;
	    cl->state |= F_LeavingComment;
	}
    } else if (cl->state & F_InCharQuote) {
	if (*in == '\\') {
	    readwhack(cl, in);
	    ++cl->charquote;
	} else if (*in == '\'') {
	    if (!cl->charquote)
		error(errBadCharLiteral);
	    else if (cl->charquote > (cl->state & F_LongChar ? 4 : 1))
                error(errBadCharLiteral);
	    cl->state |= F_LeavingCharQuote;
            cl->state &= ~F_LongChar;
	} else {
	    ++cl->charquote;
	}
    } else if (cl->state & F_InString) {
	if (*in == '\\')
	    readwhack(cl, in);
	else if (*in == '"')
	    cl->state |= F_LeavingString;
    } else {
	if (*in == '/') {
	    if (in[1] == '/') {
		++cl->charcount;
		cl->state |= F_In99Comment | F_Whitespace;
	    } else if (in[1] == '*') {
		++cl->charcount;
		cl->state |= F_InComment | F_Whitespace;
	    }
	}
	if (!(cl->state & (F_Whitespace | F_Seen1st))) {
	    cl->state |= F_Seen1st;
	    if (*in == '#') {
		cl->state |= F_Preprocess;
            } else if (in[0] == '%' && in[1] == ':') {
		cl->state |= F_Preprocess;
                ++cl->charcount;
            }
	}
	if (!(cl->state & F_Whitespace)) {
	    if (*in == '\'') {
		cl->state |= F_InCharQuote;
		cl->charquote = 0;
	    } else if (*in == '"') {
		cl->state |= F_InString;
	    } else if (*in == 'L') {
		if (in[1] == '\'') {
		    ++cl->charcount;
		    cl->state |= F_InCharQuote | F_LongChar;
		    cl->charquote = 0;
		} else if (in[1] == '"') {
		    ++cl->charcount;
		    cl->state |= F_InString;
		}
	    } else if (*in == '(') {
		++cl->parenlevel;
	    } else if (*in == ')') {
		--cl->parenlevel;
	    }
	}
    }
    return input;
}

/* Begin examining a new line of input. The return value is a pointer
 * to the string buffer containing the line.
 */
char const *beginline(clexer *cl, char const *input)
{
    return examinechar(cl, input);
}

/* Advances one character token in the input. Does nothing if the
 * lexer has already reached the end of the current line.
 */
char const *nextchar(clexer *cl, char const *input)
{
    return endoflinep(cl) ? input : examinechar(cl, input + cl->charcount);
}

/* Advances n character tokens in the input.
 */
char const *nextchars(clexer *cl, char const *input, int n)
{
    for ( ; n && !endoflinep(cl) ; --n)
	input = examinechar(cl, input + cl->charcount);
    return input;
}

/* Advances past any whitespace at the current position.
 */
char const *skipwhite(clexer *cl, char const *input)
{
    for ( ; whitespacep(cl) ; input = nextchar(cl, input)) ;
    return input;
}

/* Advances to the end of the current line.
 */
char const *restofline(clexer *cl, char const *input)
{
    while (!endoflinep(cl))
	input = nextchar(cl, input);
    return input;
}

/* Advances past the preprocessor statement at the current position,
 * and returns the statement type in cmdid.
 */
char const *getpreprocessorcmd(clexer *cl, char const *line,
			       enum ppcmd *cmdid)
{
    char const *begin, *end;
    size_t size;
    int n;

    if (!preproclinep(cl)) {
	*cmdid = cmdNone;
	return line;
    }
    begin = skipwhite(cl, line);
    if (endoflinep(cl)) {
	*cmdid = cmdNone;
	return begin;
    }

    for (end = begin ; _issym(*end) ; end = nextchar(cl, end)) ;
    size = (size_t)(end - begin);
    *cmdid = cmdOther;
    for (n = 0 ; n < sizearray(ppstatements) ; ++n) {
	if (size == strlen(ppstatements[n].statement) &&
			!memcmp(ppstatements[n].statement, begin, size)) {
	    *cmdid = ppstatements[n].id;
	    break;
	}
    }

    return skipwhite(cl, end);
}

/* Close the current line of input.
 */
void endline(clexer *cl)
{
    cl->state &= ~(F_EndOfLine | F_Seen1st | F_In99Comment | F_Preprocess);
    cl->charcount = 0;
}

/* Marks the end of an input file. Final errors are detected and the
 * lexer is re-initialized.
 */
void endstream(clexer *cl)
{
    if (cl->state & F_InCharQuote)
	error(errOpenCharLiteral);
    else if (cl->state & F_InString)
	error(errOpenStringLiteral);
    else if (cl->state & F_InComment)
	error(errOpenComment);
    cl->state = 0;
    cl->charquote = 0;
    cl->parenlevel = 0;
    cl->charcount = 0;
}
