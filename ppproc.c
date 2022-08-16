/* ppproc.c: Copyright (C) 2011-2022 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"
#include "types.h"
#include "error.h"
#include "symset.h"
#include "mstr.h"
#include "clexer.h"
#include "exptree.h"
#include "ppproc.h"

/* Maximum nesting level of #if statements.
 */
#define STACK_SIZE 1024

/* State flags tracking the current state of ppproc.
 */
#define F_If            0x0001          /* inside a #if section */
#define F_Else          0x0002          /* inside a #else section */
#define F_Elif          0x0004          /* inside a #elif section */
#define F_Ifdef         0x0008          /* #if is actually #ifdef/#ifndef */
#define F_Ours          0x0010          /* guarded by user-specified symbol */
#define F_Copy          0x0020          /* section is passed to output */
#define F_IfModify      0x0040          /* modified #if expression */
#define F_ElseModify    0x0080          /* modified #elif expression */

/* Return codes for the seqif() function.
 */
enum status
{
    statError, statDefined, statUndefined, statPartDefined, statUnaffected
}; 

/* The partial preprocessor.
 */
struct ppproc {
    clexer     *cl;                     /* the lexer */
    symset const *defs;                 /* list of defined symbols */
    symset const *undefs;               /* list of undefined symbols */
    mstr       *line;                   /* the current line of input */
    int         copy;                   /* true if input is going to output */
    int         absorb;                 /* true if input is being suppressed */
    int         level;                  /* current nesting level */
    int         stack[STACK_SIZE];      /* state flags for each level */
};

/* This global flag controls trigraph handling.
 */
static int trigraphsenabled = FALSE;

/* Allocates a partial preprocessor object.
 */
ppproc *initppproc(symset const *defs, symset const *undefs)
{
    ppproc *ppp;

    ppp = allocate(sizeof *ppp);
    ppp->cl = initclexer();
    ppp->defs = defs;
    ppp->undefs = undefs;
    ppp->line = initmstr();
    return ppp;
}

/* Deallocates the partial preprocessor object.
 */
void freeppproc(ppproc *ppp)
{
    freeclexer(ppp->cl);
    freemstr(ppp->line);
    deallocate(ppp);
}

/* Enable and disable trigraph handling.
 */
void enabletrigraphs(int flag)
{
    trigraphsenabled = flag;
}

/* Set the state appropriate for the beginning of a file.
 */
static void beginfile(ppproc *ppp)
{
    ppp->level = -1;
    ppp->copy = TRUE;
    ppp->absorb = FALSE;
}

/* Mark the ppproc as having reached the end of the current file.
 */
static void endfile(ppproc *ppp)
{
    endstream(ppp->cl);
    if (ppp->level != -1)
        error(errOpenIf);
    erasemstr(ppp->line);
}

/* Partially preprocesses a #if expression. ifexp points to the text
 * immediately following the #if. The function seeks to the end of the
 * expression and evaluates it. The return value points to the text
 * immediately following the expression. If the expression has a
 * defined state, status receives either statDefined or statUndefined.
 * If the expression's contents are disjoint from the defined and
 * undefined symbols, status receives statUnaffected. Otherwise the
 * expression is in a partial state, in which case status receives
 * statPartDefined, and the original string is modified so as to
 * remove the parts of the expression that have a defined state.
 */
static char const *seqif(ppproc *ppp, char *ifexp, enum status *status)
{
    exptree *tree;
    char const *ret;
    char *str;
    int defined, n;

    tree = initexptree();
    *status = statUnaffected;

    n = geterrormark();
    ret = parseexptree(tree, ppp->cl, ifexp);
    if (errorsincemark(n)) {
        *status = statError;
        goto quit;
    }

    n = markdefined(tree, ppp->defs, TRUE) +
        markdefined(tree, ppp->undefs, FALSE);
    if (n) {
        *status = evaltree(tree, &defined) ? statDefined : statUndefined;
        if (!defined) {
            *status = statPartDefined;
            str = allocate(strlen(ifexp) + 1);
            n = unparseevaluated(tree, str);
            ret = editmstr(ppp->line, ifexp, getexplength(tree), str, n) + n;
            deallocate(str);
        }
    }

  quit:
    freeexptree(tree);
    return ret;
}

/* Partially preprocesses the current line of input. If the input
 * contains a preprocessor statement, the state of ppproc is updated
 * to reflect the current section, and if necessary the line of input
 * will be altered for output. This is where the sausage is made.
 */
static void seq(ppproc *ppp)
{
    char const *input;
    char const *cmd;
    char const *cmdend;
    enum status status;
    int incomment;
    enum ppcmd id;
    int size, n;

    incomment = ccommentp(ppp->cl);
    ppp->absorb = FALSE;
    input = beginline(ppp->cl, getmstrbuf(ppp->line));
    while (!preproclinep(ppp->cl)) {
        if (endoflinep(ppp->cl))
            return;
        input = nextchar(ppp->cl, input);
    }

    cmd = skipwhite(ppp->cl, nextchar(ppp->cl, input));
    input = getpreprocessorcmd(ppp->cl, cmd, &id);

    switch (id) {
      case cmdIfdef:
      case cmdIfndef:
        if (ppp->level + 1 >= sizearray(ppp->stack)) {
            error(errIfsTooDeep);
            break;
        }
        ++ppp->level;
        ppp->stack[ppp->level] = F_If | F_Ifdef;
        if (!ppp->copy) {
            input = restofline(ppp->cl, input);
            break;
        }
        ppp->stack[ppp->level] |= F_Copy;
        size = getidentifierlength(input);
        if (!size) {
            error(errEmptyIf);
            break;
        }
        if (findsymbolinset(ppp->defs, input, NULL))
            status = statDefined;
        else if (findsymbolinset(ppp->undefs, input, NULL))
            status = statUndefined;
        else
            status = statUnaffected;
        cmdend = nextchars(ppp->cl, input, size);
        input = skipwhite(ppp->cl, cmdend);
        if (!endoflinep(ppp->cl)) {
            error(errSyntax);
            break;
        }
        if (status != statUnaffected) {
            ppp->absorb = TRUE;
            ppp->stack[ppp->level] |= F_Ours;
            if (id == cmdIfdef)
                ppp->copy = status == statDefined;
            else
                ppp->copy = status == statUndefined;
        }
        break;

      case cmdIf:
        if (ppp->level + 1 >= sizearray(ppp->stack)) {
            error(errIfsTooDeep);
            break;
        }
        ++ppp->level;
        ppp->stack[ppp->level] = F_If | (ppp->copy ? F_Copy : 0);
        if (!ppp->copy) {
            input = restofline(ppp->cl, input);
            break;
        }
        cmdend = seqif(ppp, (char*)input, &status);
        if (status == statError)
            break;
        input = skipwhite(ppp->cl, cmdend);
        if (!endoflinep(ppp->cl)) {
            error(errIfSyntax);
            break;
        }
        if (status == statDefined || status == statUndefined) {
            ppp->absorb = TRUE;
            ppp->stack[ppp->level] |= F_Ours;
            ppp->copy = status == statDefined;
        }
        break;

      case cmdElse:
        if (ppp->level < 0 || (ppp->stack[ppp->level] & F_Else)) {
            error(errDanglingElse);
            break;
        }
        ppp->stack[ppp->level] |= F_Else;
        if (!endoflinep(ppp->cl)) {
            error(errSyntax);
            break;
        }
        cmdend = input;
        if (ppp->stack[ppp->level] & F_Ours) {
            ppp->copy = !ppp->copy;
            ppp->absorb = TRUE;
            n = ppp->level;
            while (ppp->stack[n] & F_Elif) {
                if (ppp->stack[n] & F_ElseModify) {
                    ppp->absorb = TRUE;
                    break;
                }
                --n;
                if (!(ppp->stack[n] & F_Ours))
                    ppp->absorb = FALSE;
            }
        }
        break;

      case cmdElifdef:
      case cmdElifndef:
        if (ppp->level < 0 || (ppp->stack[ppp->level] & F_Else)) {
            error(errDanglingElse);
            break;
        } else if (ppp->level + 1 >= sizearray(ppp->stack)) {
            error(errIfsTooDeep);
            break;
        }
        if (!(ppp->stack[ppp->level] & F_Ifdef))
            error(errElifdefWithIf);
        ppp->stack[ppp->level] |= F_Else;
        if (ppp->stack[ppp->level] & F_Ours) {
            ppp->copy = !ppp->copy;
            ppp->absorb = TRUE;
            n = ppp->level;
            while (ppp->stack[n] & F_Elif) {
                if (ppp->stack[n] & F_ElseModify) {
                    ppp->absorb = TRUE;
                    break;
                }
                --n;
                if (!(ppp->stack[n] & F_Ours))
                    ppp->absorb = FALSE;
            }
        }
        ++ppp->level;
        ppp->stack[ppp->level] = F_If | F_Elif | F_Ifdef;
        cmdend = input;
        if (!ppp->copy) {
            input = restofline(ppp->cl, input);
            break;
        }
        ppp->stack[ppp->level] |= F_Copy;
        size = getidentifierlength(input);
        if (!size) {
            error(errEmptyIf);
            break;
        }
        if (findsymbolinset(ppp->defs, input, NULL))
            status = statDefined;
        else if (findsymbolinset(ppp->undefs, input, NULL))
            status = statUndefined;
        else
            status = statUnaffected;
        cmdend = nextchars(ppp->cl, input, size);
        input = skipwhite(ppp->cl, cmdend);
        if (!endoflinep(ppp->cl)) {
            error(errSyntax);
            break;
        }
        if (status == statUnaffected) {
            ppp->absorb = FALSE;
            n = ppp->level;
            while (ppp->stack[n] & F_Elif) {
                --n;
                if (!(ppp->stack[n] & F_Ours)) {
                    n = -1;
                    break;
                }
            }
            if (n >= 0) {
                editmstr(ppp->line, cmd, 2, "", 0);
                ppp->stack[ppp->level] |= F_IfModify;
            }
        } else {
            if (id == cmdElifdef)
                ppp->copy = status == statDefined;
            else
                ppp->copy = status == statUndefined;
            ppp->absorb = TRUE;
            if (ppp->copy) {
                n = ppp->level;
                while (ppp->stack[n] & F_Elif) {
                    --n;
                    if (!(ppp->stack[n] & F_Ours)) {
                        editmstr(ppp->line, cmd, cmdend - cmd, "else", 4);
                        ppp->stack[ppp->level] |= F_ElseModify;
                        ppp->absorb = FALSE;
                        break;
                    }
                }
            }
            ppp->stack[ppp->level] |= F_Ours;
        }
        break;

      case cmdElif:
        if (ppp->level < 0 || !(ppp->stack[ppp->level] & F_If)
                           || (ppp->stack[ppp->level] & F_Else)) {
            error(errDanglingElse);
            break;
        } else if (ppp->level + 1 >= sizearray(ppp->stack)) {
            error(errIfsTooDeep);
            break;
        }
        if (ppp->stack[ppp->level] & F_Ifdef)
            error(errElifWithIfdef);
        ppp->stack[ppp->level] |= F_Else;
        if (ppp->stack[ppp->level] & F_Ours)
            ppp->copy = !ppp->copy;
        ++ppp->level;
        ppp->stack[ppp->level] = F_If | F_Elif | (ppp->copy ? F_Copy : 0);
        if (!ppp->copy) {
            input = restofline(ppp->cl, input);
            break;
        }
        cmdend = seqif(ppp, (char*)input, &status);
        if (status == statError)
            break;
        input = skipwhite(ppp->cl, cmdend);
        if (!endoflinep(ppp->cl)) {
            error(errIfSyntax);
            break;
        }
        if (status == statUndefined) {
            ppp->copy = FALSE;
            ppp->absorb = TRUE;
            ppp->stack[ppp->level] |= F_Ours;
        } else if (status == statDefined) {
            ppp->absorb = TRUE;
            n = ppp->level;
            while (ppp->stack[n] & F_Elif) {
                --n;
                if (!(ppp->stack[n] & F_Ours)) {
                    editmstr(ppp->line, cmd, cmdend - cmd, "else", 4);
                    ppp->stack[ppp->level] |= F_ElseModify;
                    ppp->absorb = FALSE;
                    break;
                }
            }
            ppp->stack[ppp->level] |= F_Ours;
        } else {
            n = ppp->level;
            while (ppp->stack[n] & F_Elif) {
                --n;
                if (!(ppp->stack[n] & F_Ours)) {
                    n = -1;
                    break;
                }
            }
            if (n >= 0) {
                editmstr(ppp->line, cmd, 2, "", 0);
                ppp->stack[ppp->level] |= F_IfModify;
            }
        }
        break;

      case cmdEndif:
        if (ppp->level < 0) {
            error(errDanglingEnd);
            break;
        }
        cmdend = input;
        if (!endoflinep(ppp->cl)) {
            error(errSyntax);
            input = restofline(ppp->cl, input);
        }
        ppp->absorb = TRUE;
        for ( ; ppp->stack[ppp->level] & F_Elif ; --ppp->level) {
            if (ppp->stack[ppp->level] & (F_IfModify | F_ElseModify))
                ppp->absorb = FALSE;
        }
        if (ppp->absorb)
            ppp->absorb = ppp->stack[ppp->level] & F_Ours;
        ppp->copy = ppp->stack[ppp->level] & F_Copy;
        --ppp->level;
        break;

      default:
        input = restofline(ppp->cl, input);
        break;
    }

    if (ppp->absorb && incomment != ccommentp(ppp->cl))
        error(errBrokenComment);
}

/* Reads the next line of source code, and applies the first two
 * phases of translation. Phase one is trigraph replacement, and phase
 * two removes backslash-newline pairs. (This code also provides an
 * extra in-between step of turning CRLF sequences into simple
 * newlines.) The resulting line is then ready for the third phase of
 * translation, namely preprocessing. The return value is zero if the
 * file has already reached the end or if the file can't be read from.
 */
static int readline(ppproc *ppp, FILE *infile)
{
    char const *p;
    int replacement;
    int back2, back1, ch;

    ch = fgetc(infile);
    if (ch == EOF)
        return 0;
    back2 = EOF;
    back1 = EOF;
    erasemstr(ppp->line);
    while (ch != EOF) {
        appendmstr(ppp->line, ch);
        if (trigraphsenabled && back2 == '?' && back1 == '?') {
            switch (ch) {
              case '=':         replacement = '#';      break;
              case '(':         replacement = '[';      break;
              case '/':         replacement = '\\';     break;
              case ')':         replacement = ']';      break;
              case '\'':        replacement = '^';      break;
              case '<':         replacement = '{';      break;
              case '!':         replacement = '|';      break;
              case '>':         replacement = '}';      break;
              case '-':         replacement = '~';      break;
              default:          replacement = 0;        break;
            }
            if (replacement) {
                p = getmstrbuf(ppp->line) + getmstrlen(ppp->line);
                altermstr(ppp->line, p - 3, 3, replacement);
                ch = replacement;
                back1 = back2 = EOF;
            }
        }
        if (back1 == '\r' && ch == '\n') {
            p = getmstrbuf(ppp->line) + getmstrlen(ppp->line);
            altermstr(ppp->line, p - 2, 2, '\n');
            back1 = back2;
            back2 = EOF;
        }
        if (ch == '\n') {
            if (back1 == '\\') {
                p = getmstrbuf(ppp->line) + getmstrlen(ppp->line);
                altermstr(ppp->line, p - 2, 2, 0);
                ch = back2;
                back1 = back2 = EOF;
            } else {
                break;
            }
        }
        back2 = back1;
        back1 = ch;
        ch = fgetc(infile);
    }

    if (ferror(infile)) {
        error(errFileIO);
        return 0;
    }
    return 1;
}

/* Outputs the partially preprocessed line to the output file,
 * assuming anything is left to be output. The return value is false
 * if an error occurs.
 */
static int writeline(ppproc *ppp, FILE *outfile)
{
    size_t size;

    if (!ppp->line)
        return 1;
    if (!ppp->copy || ppp->absorb)
        return 1;

    size = getmstrbaselen(ppp->line);
    if (size) {
        if (fwrite(getmstrbase(ppp->line), size, 1, outfile) != 1) {
            seterrorfile(NULL);
            error(errFileIO);
            return 0;
        }
    }
    return 1;
}

/* Increments the line number count, checking for embedded line break
 * characters.
 */
static void advanceline(mstr const *line)
{
    char const *p;

    for (p = getmstrbuf(line) - 1 ; p ; p = strchr(p + 1, '\n'))
        nexterrorline();
}

/* Partially preprocesses each line of infile and writes the results
 * to outfile.
 */
void partialpreprocess(ppproc *ppp, FILE *infile, FILE *outfile)
{
    beginfile(ppp);
    seterrorline(1);
    while (readline(ppp, infile)) {
        seq(ppp);
        endline(ppp->cl);
        if (!writeline(ppp, outfile))
            break;
        advanceline(ppp->line);
    }
    seterrorline(0);
    endfile(ppp);
}
