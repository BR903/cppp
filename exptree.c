#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	"main.h"
#include	"error.h"
#include	"strset.h"
#include	"clex.h"
#include	"exptree.h"

enum
{
    expNone = 0, expConstant, expMacro, expParamMacro, expDefined,
    expOperator
};

enum
{
    opNone = 0, opLogNot, opBitNot, opPositive, opNegative,
    opPrefixCount,
    opLeftShift = opPrefixCount, opRightShift, opEqual, opInequal,
    opLogAnd, opLogOr, opMultiply, opDivide, opModulo,
    opGreater, opLesser, opGreaterEqual, opLesserEqual,
    opAdd, opSubtract, opBitAnd, opBitXor, opBitOr,
    opConditional, opComma, opCount
};

#define	_(s, i, p, d)	{ s, sizeof s - 1, i, p, d }
static struct { char const *op; int size; int id; int prec; int l2r; } const
	prefixop[] = {
		_("!", opLogNot, 14, FALSE),	_("~", opBitNot, 14, FALSE),
		_("+", opPositive, 14, FALSE),	_("-", opNegative, 14, FALSE)
	},
	infixop[] = {
		_("<<", opLeftShift, 11, TRUE),
		_(">>", opRightShift, 11, TRUE),
		_("==", opEqual, 9, TRUE),	_("!=", opInequal, 9, TRUE),
		_("<=", opLesserEqual, 10, TRUE),
		_(">=", opGreaterEqual, 10, TRUE),
		_("&&", opLogAnd, 5, TRUE),	_("||", opLogOr, 4, TRUE),
		_("*", opMultiply, 13, TRUE),
		_("/", opDivide, 13, TRUE),	_("%", opModulo, 13, TRUE),
		_("<", opGreater, 10, TRUE),	_(">", opLesser, 10, TRUE),
		_("+", opAdd, 12, TRUE),	_("-", opSubtract, 12, TRUE),
		_("&", opBitAnd, 8, TRUE),	_("|", opBitOr, 6, TRUE),
		_("^", opBitXor, 7, TRUE),
		_("?", opConditional, 3, FALSE),
		_(",", opComma, 1, TRUE)
	};
#undef _


void initexptree(exptree *t, errhandler *err)
{
    t->childcount = 0;
    t->exp = expNone;
    t->valued = FALSE;
    t->begin = NULL;
    t->end = NULL;
    t->err = err;
}

void cleartree(exptree *t, int discard)
{
    int n;

    if (discard) {
	for (n = 0 ; n < t->childcount ; ++n)
	    cleartree(t->child[n], TRUE);
    }
    t->childcount = 0;
    t->exp = expNone;
    t->valued = FALSE;
    t->begin = NULL;
    t->end = NULL;
}


int valuedp(exptree const *t)
{
    return t->exp != expNone && t->exp != expOperator && t->valued;
}

int getexplength(exptree const *t)
{
    return t->exp == expNone ? 0 : (int)(t->end - t->begin);
}

char const *getexp(exptree const *t)
{
    return t->exp == expNone ? NULL : t->begin;
}

int setvalue(exptree *t, int valued, long value)
{
    if (t->exp == expNone || t->exp == expOperator)
	return FALSE;

    t->valued = valued;
    if (valued)
	t->value = value;
    return TRUE;
}


int addchild(exptree *t, exptree *exp, int pos)
{
    int	n;

    if (t->childcount == MAX_EXPTREECHILDREN || !exp)
	return FALSE;
    if (pos < -1 || pos > t->childcount)
	return FALSE;

    if (pos == -1 || pos == t->childcount)
	t->child[t->childcount] = exp;
    else {
	for (n = t->childcount ; n > pos ; --n)
	    t->child[n] = t->child[n - 1];
	t->child[pos] = exp;
    }
    ++t->childcount;
    return TRUE;
}

exptree *addnewchild(exptree *t, int pos)
{
    exptree    *child;

    child = xmalloc(sizeof *child);
    initexptree(child, t->err);
    if (!addchild(t, child, pos)) {
	free(child);
	return NULL;
    }
    return child;
}

exptree *removechild(exptree *t, int pos)
{
    exptree    *child;
    int		n;

    if (!t->childcount)
	return NULL;
    if (pos < -1 || pos >= t->childcount)
	return NULL;

    if (pos >= 0) {
	child = t->child[pos];
	for (n = pos + 1 ; n < t->childcount ; ++n)
	    t->child[n] = t->child[n + 1];
    } else
	child = t->child[t->childcount - 1];

    --t->childcount;
    return child;
}

int discardchild(exptree *t, int pos)
{
    int n;

    if (!t->childcount)
	return FALSE;
    if (pos < -1 || pos >= t->childcount)
	return FALSE;

    if (pos == -1)
	pos = t->childcount - 1;
    free(t->child[n]);
    return removechild(t, pos) != NULL;
}

#if 0
int shufflechild(exptree *t, int pos, int topos)
{
    exptree	temp;
    exptree    *child;

    if (pos < 0 || pos >= t->childcount)
	return FALSE;
    if (t->child[pos]->childcount == MAX_EXPTREECHILDREN)
	return FALSE;

    temp = *t;
    child = removechild(t, pos);
    if (!child)
	return FALSE;
    *t = *child;
    *child = temp;
    addchild(t, child, topos);
    return TRUE;
}
#endif

exptree *duptree(exptree const *t)
{
    exptree    *tnew, *child;
    int		n;

    tnew = xmalloc(sizeof *tnew);
    initexptree(tnew, t->err);
    *tnew = *t;
    cleartree(tnew, FALSE);
    for (n = 0 ; n < t->childcount ; ++n) {
	if (!(child = duptree(t->child[n]))) {
	    cleartree(tnew, TRUE);
	    free(tnew);
	    return NULL;
	}
	addchild(tnew->child[n], child, -1);
    }
    return tnew;
}


char const *parseconstant(exptree *t, clexer *cl, char const *input)
{
    char const *in = input;
    int		size, paren, mark;

    mark = geterrormark(t->err);
    t->begin = in;
    if (charquotep(cl)) {
	t->exp = expConstant;
	while (!endoflinep(cl) && charquotep(cl))
	    in = nextchar(cl, in);
	t->end = in;
	t->valued = FALSE;
	in = skipwhite(cl, in);
    } else if (!memcmp(in, "defined", 7)) {
	t->exp = expDefined;
	in = skipwhite(cl, nextchars(cl, in, 7));
	paren = *in == '(';
	if (paren)
	    in = skipwhite(cl, nextchar(cl, in));
	size = getidentifierlength(in);
	if (!size) {
	    error(t->err, errDefinedSyntax);
	    goto failure;
	}
	t->identifier = in;
	in = nextchars(cl, in, size);
	if (paren) {
	    in = skipwhite(cl, in);
	    if (*in != ')') {
		error(t->err, errDefinedSyntax);
		goto failure;
	    }
	    in = nextchar(cl, in);
	}
	t->valued = FALSE;
	t->end = in;
	in = skipwhite(cl, in);
    } else if (isdigit(*in)) {
	t->exp = expConstant;
	if (*in == '0') {
	    in = nextchar(cl, in);
	    if (tolower(*in) == 'x') {
		do
		    in = nextchar(cl, in);
		while (isxdigit(*in));
	    } else {
		while (*in >= '0' && *in <= '7')
		    in = nextchar(cl, in);
	    }
	} else {
	    do
		in = nextchar(cl, in);
	    while (isdigit(*in));
	}
	if (toupper(*in) == 'L') {
	    in = nextchar(cl, in);
	    if (toupper(*in) == 'L')
		in = nextchar(cl, in);
	    if (toupper(*in) == 'U')
		in = nextchar(cl, in);
	} else if (toupper(*in) == 'U') {
	    in = nextchar(cl, in);
	    if (toupper(*in) == 'L') {
		in = nextchar(cl, in);
		if (toupper(*in) == 'L')
		    in = nextchar(cl, in);
	    }
	}
	t->valued = FALSE;
	t->end = in;
	in = skipwhite(cl, in);
    } else if (_issym(*in)) {
	do
	    in = nextchar(cl, in);
	while (_issym(*in));
	t->end = in;
	in = skipwhite(cl, in);
	if (*in == '(') {
	    t->exp = expParamMacro;
	    paren = cl->parenlevel;
	    do {
		in = nextchar(cl, in);
		if (endoflinep(cl)) {
		    error(t->err, errOpenParenthesis);
		    goto failure;
		}
	    } while (cl->parenlevel >= paren);
	    t->valued = FALSE;
	    t->end = in;
	    in = skipwhite(cl, in);
	} else
	    t->exp = expMacro;
    } else {
	error(t->err, errSyntax);
	goto failure;
    }

    if (!errorsincemark(t->err, mark))
	return in;

  failure:
    t->exp = expNone;
    t->end = in;
    return in;
}

char const *parseexp(exptree *t, clexer *cl, char const *input, int prec)
{
    char const *in = input;
    char const *temp;
    exptree    *x;
    int		found, n;

    if (t->exp != expNone)
	return input;

    if (*in == '(') {
	temp = in;
	in = skipwhite(cl, nextchar(cl, in));
	in = parseexp(t, cl, in, 0);
	if (t->exp == expNone) {
	    error(t->err, errSyntax);
	    goto failure;
	} else if (*in != ')') {
	    error(t->err, errOpenParenthesis);
	    goto failure;
	}
	t->begin = temp;
	in = nextchar(cl, in);
	t->end = in;
	in = nextchar(cl, in);
    } else {
	found = FALSE;
	for (n = 0 ; n < sizearray(prefixop) ; ++n) {
	    if (!memcmp(in, prefixop[n].op, prefixop[n].size)) {
		found = TRUE;
		break;
	    }
	}
	if (found) {
	    if (prefixop[n].prec < prec) {
		error(t->err, errMissingOperand);
		goto failure;
	    }
	    t->exp = expOperator;
	    t->opid = prefixop[n].id;
	    t->begin = in;
	    in = nextchars(cl, in, prefixop[n].size);
	    in = skipwhite(cl, in);
	    x = addnewchild(t, -1);
	    in = parseexp(x, cl, in, prefixop[n].prec);
	    if (x->exp == expNone) {
		error(t->err, errSyntax);
		goto failure;
	    }
	    t->end = x->end;
	} else {
	    in = parseconstant(t, cl, in);
	    if (t->exp == expNone) {
		error(t->err, errSyntax);
		goto failure;
	    }
	}
    }

    for (;;) {
	found = FALSE;
	for (n = 0 ; n < sizearray(infixop) ; ++n) {
	    if (!memcmp(in, infixop[n].op, infixop[n].size)) {
		found = TRUE;
		break;
	    }
	}
	if (!found || infixop[n].prec < prec
		   || (infixop[n].prec == prec && infixop[n].l2r))
	    break;
	in = nextchars(cl, in, infixop[n].size);
	in = skipwhite(cl, in);
	x = xmalloc(sizeof *x);
	initexptree(x, t->err);
	temp = t->begin;
	*x = *t;
	cleartree(t, FALSE);
	t->exp = expOperator;
	t->opid = infixop[n].id;
	t->begin = temp;
	addchild(t, x, -1);
	x = addnewchild(t, -1);
	if (t->opid == opConditional) {
	    in = parseexp(x, cl, in, infixop[n].prec);
	    if (x->exp == expNone) {
		error(t->err, errSyntax);
		goto failure;
	    }
	    if (*in != ':') {
		error(t->err, errSyntax);
		goto failure;
	    }
	    in = skipwhite(cl, nextchar(cl, in));
	    x = addnewchild(t, -1);
	}
	in = parseexp(x, cl, in, infixop[n].prec);
	if (x->exp == expNone) {
	    error(t->err, errSyntax);
	    goto failure;
	}
	t->end = x->end;
    }
    return in;

  failure:
    t->exp = expNone;
    return in;
}

int markdefined(exptree *t, strset *set, int defined)
{
    int count, n;

    count = 0;
    for (n = 0 ; n < t->childcount ; ++n)
	count += markdefined(t->child[n], set, defined);
    if (!t->valued) {
	if (t->exp == expDefined) {
	    if (findsymbolinset(set, t->identifier)) {
		t->valued = TRUE;
		t->value = defined ? 1L : 0L;
		++count;
	    }
	} else if (t->exp == expMacro) {
	    if (findsymbolinset(set, t->begin)) {
		t->valued = TRUE;
		t->value = defined ? 1L : 0L;
		++count;
	    }
	}
    }
    return count;
}

long evaltree(exptree *t, int *defined)
{
    long	val1, val2;
    int		valued;

    if (t->exp == expNone) {
	if (defined)
	    *defined = FALSE;
	return 0L;
    }
    if (t->exp != expOperator) {
	if (defined)
	    *defined = t->valued;
	return t->valued ? t->value : 0L;
    }

    if (t->opid < opPrefixCount) {
	val1 = evaltree(t->child[0], &valued);
	if (valued) {
	    switch (t->opid) {
	      case opLogNot:	val1 = !val1;	break;
	      case opBitNot:	val1 = ~val1;	break;
	      case opPositive:	val1 = +val1;	break;
	      case opNegative:	val1 = -val1;	break;
	    }
	}
	goto done;
    }
    val1 = evaltree(t->child[0], &valued);
    if (t->opid == opComma) {
	val1 = evaltree(t->child[1], &valued);
	goto done;
    } else if (t->opid == opConditional) {
	if (valued)
	    val1 = evaltree(t->child[val1 ? 1 : 2], &valued);
	goto done;
    } else if (t->opid == opLogAnd) {
	if (valued) {
	    if (val1)
		val1 = evaltree(t->child[1], &valued);
	} else {
	    val1 = evaltree(t->child[1], &valued);
	    if (valued && val1)
		valued = FALSE;
	}
	goto done;
    } else if (t->opid == opLogOr) {
	if (valued) {
	    if (!val1)
		val1 = evaltree(t->child[1], &valued);
	} else {
	    val1 = evaltree(t->child[1], &valued);
	    if (valued && !val1)
		valued = FALSE;
	}
	goto done;
    }
    if (!valued)
	goto done;

    val2 = evaltree(t->child[1], &valued);
    if (valued) {
	switch (t->opid) {
	  case opLeftShift:	val1 = val1 << (int)val2;	break;
	  case opRightShift:	val1 = val1 >> (int)val2;	break;
	  case opLesserEqual:	val1 = val1 <= val2;		break;
	  case opGreaterEqual:	val1 = val1 >= val2;		break;
	  case opEqual:		val1 = val1 == val2;		break;
	  case opInequal:	val1 = val1 != val2;		break;
	  case opMultiply:	val1 = val1 * val2;		break;
	  case opDivide:	val1 = val1 / val2;		break;
	  case opModulo:	val1 = val1 % val2;		break;
	  case opGreater:	val1 = val1 > val2;		break;
	  case opLesser:	val1 = val1 < val2;		break;
	  case opAdd:		val1 = val1 + val2;		break;
	  case opSubtract:	val1 = val1 - val2;		break;
	  case opBitAnd:	val1 = val1 & val2;		break;
	  case opBitXor:	val1 = val1 ^ val2;		break;
	  case opBitOr:		val1 = val1 | val2;		break;
	}
    }

  done:
    t->valued = valued;
    if (valued)
	t->value = val1;
    if (defined)
	*defined = valued;
    return defined ? val1 : 0L;
}

int unparseevaluated(exptree const *t, char *buffer)
{
    char const *src;
    char       *buf;
    int		size, n;

    if (t->exp == expNone)
	return 0;
    if (t->exp != expOperator) {
	if (t->valued)
	    size = sprintf(buffer, "%ld", t->value);
	else {
	    size = getexplength(t);
	    memcpy(buffer, getexp(t), size);
	}
	return size;
    }
    if (t->opid == opConditional) {
	if (t->child[0]->valued)
	    return unparseevaluated(t->child[t->child[0]->value ? 1 : 2],
				    buffer);
    } else if (t->opid == opLogAnd || t->opid == opLogOr) {
	if (t->child[0]->valued)
	    return unparseevaluated(t->child[1], buffer);
	else if (t->child[1]->valued)
	    return unparseevaluated(t->child[0], buffer);
    }

    buf = buffer;
    src = t->begin;
    for (n = 0 ; n < t->childcount ; ++n) {
	size = (int)(t->child[n]->begin - src);
	if (size) {
	    memcpy(buf, src, size);
	    buf += size;
	}
	buf += unparseevaluated(t->child[n], buf);
	src = t->child[n]->end;
    }
    size = (int)(t->end - src);
    if (size) {
	memcpy(buf, src, size);
	buf += size;
    }
    return (int)(buf - buffer);
}

#define	MAXINLEVEL	32

static int gethorzlength(exptree const *t)
{
    int	pos, n;

    if (t->childcount) {
	pos = -1;
	for (n = 0 ; n < t->childcount ; ++n) 
	    pos += 1 + gethorzlength(t->child[n]);
    } else if (t->exp != expNone)
	pos = t->end - t->begin;
    else
	pos = 6;

    return pos;
}

void drawtree(exptree const *t)
{
    exptree const      *set1[MAXINLEVEL];
    exptree const      *set2[MAXINLEVEL];
    exptree const      *x, *child;
    char const	       *s;
    int			tab1[MAXINLEVEL];
    int			tab2[MAXINLEVEL];
    int			count1, count2;
    int			size, pos, len, tab, node;
    int			n, m, l;

    set1[0] = t;
    tab1[0] = 0;
    count1 = 1;
    while (count1) {
	count2 = 0;
	tab2[0] = 0;
	pos = tab = 0;
	for (node = 0 ; node < count1 ; ++node) {
	    if (!set1[node]) {
		tab = tab1[node];
		continue;
	    }
	    x = set1[node];
	    len = tab = tab1[node];
	    m = (x->childcount + 1) / 2 - 1;
	    for (n = 0 ; n < x->childcount ; ++n) {
		child = x->child[n];
		l = gethorzlength(x->child[n]) + 1;
		set2[count2] = x->child[n];
		tab2[count2] = len;
		++count2;
		len += l;
		if (n == m) {
		    tab = len - 1;
		    if (x->childcount & 1)
			tab -= l / 2;
		}
	    }

	    s = NULL;
	    size = gethorzlength(x);
	    if (x->exp == expOperator) {
		for (n = 0 ; n < sizearray(prefixop) ; ++n) {
		    if (x->opid == prefixop[n].id) {
			s = prefixop[n].op;
			size = prefixop[n].size;
			break;
		    }
		}
		if (!s) {
		    for (n = 0 ; n < sizearray(infixop) ; ++n) {
			if (x->opid == infixop[n].id) {
			    s = infixop[n].op;
			    size = infixop[n].size;
			    break;
			}
		    }
		}
	    } else if (x->exp)
		s = x->begin;
	    else
		s = "(none)";
	    for (len = tab - size / 2 ; pos < len ; ++pos)
		fputc(' ', stdout);
	    fwrite(s, size, 1, stdout);
	    fputc(' ', stdout);
	    pos += size + 1;
	}
	if (pos) {
	    fputc('\n', stdout);
	    pos = 0;
	}
	for (n = 0 ; n < count2 ; ++n) {
	    set1[n] = set2[n];
	    tab1[n] = tab2[n];
	}
	count1 = count2;
    }
}
