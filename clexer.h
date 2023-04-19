/* clexer.h: Copyright (C) 2011-2022 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#ifndef _clexer_h_
#define _clexer_h_

/*
 * The C lexer object does basic lexical analysis on the input stream.
 * It knows enough C syntax to find the lines containing preprocessor
 * statements. The C lexer works by feeding it input text, and
 * advancing it through each character of input.
 */

#include "types.h"

/* A list of identifiers for the preprocessor commands that the
 * program cares about.
 */
enum ppcmd
{
    cmdNone = 0,
    cmdDefine,
    cmdElif, cmdElifdef, cmdElifndef, cmdElse,
    cmdEndif, cmdIf,
    cmdIfdef, cmdIfndef,
    cmdUndef,
    cmdOther
};

/* Returns the length of the C identifier located at input, or zero if
 * input does not point to a valid C identifier.
 */
extern int getidentifierlength(char const *input);

/* Creates a new C lexer.
 */
extern clexer *initclexer(void);

/* Deallocates a lexer.
 */
extern void freeclexer(clexer *cl);

/* Enable and disable error reporting for character literals that
 * contain more than one characters. Error reporting is enabled by
 * default.
 */
extern void allowmultichars(int flag);

/* These functions all return true or false depending on what the
 * lexer has last examined.
 */
extern int endoflinep(clexer const *cl);
extern int whitespacep(clexer const *cl);
extern int charquotep(clexer const *cl);
extern int ccommentp(clexer const *cl);
extern int preproclinep(clexer const *cl);

/* Returns the current number of nested parentheses.
 */
extern int getparenlevel(clexer const *cl);

/* Begins lexing a new line of input. The return value is a pointer to
 * the string buffer containing the line, which can be used as inputs
 * to the functions below.
 */
extern char const *beginline(clexer *cl, char const *line);

/* Examines the first character token in input, and returns a pointer
 * to the byte immediately following.
 */
extern char const *nextchar(clexer *cl, char const *input);

/* Examines n character tokens in input, updating state along the way,
 * and returns a pointer to the byte immediately following them.
 */
extern char const *nextchars(clexer *cl, char const *input, int skip);

/* Examines all character tokens in input until reaching the end of
 * the line.
 */
extern char const *restofline(clexer *cl, char const *input);

/* Examines characters tokens until a non-whitespace character is
 * found.
 */
extern char const *skipwhite(clexer *cl, char const *input);

/* Mark the beginning of a new line. If input is not null, it will
 * be used as the new line; otherwise, the lexer will take the next
 * input as the beginning of the new line.
 */
extern char const *nextline(clexer *cl, char const *input);

/* Mark the end of the current input file.
 */
extern void endstream(clexer *cl);

/* Examines character tokens until the end of the preprocessor
 * statement is found. The value identifying the preprocessor statement
 * is returned via cmdid.
 */
extern char const *getpreprocessorcmd(clexer *cl, char const *input,
                                      enum ppcmd *cmdid);

/* Mark the end of the current line of input. This function should be
 * called before the next call to beginline().
 */
extern void endline(clexer *cl);

/* Mark the end of the current input file.
 */
extern void endstream(clexer *cl);

#endif
