#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<stdarg.h>
#include	"main.h"
#include	"unixisms.h"
#include	"error.h"
#include	"strset.h"
#include	"ppproc.h"

static char const *const yowzitch =
	"cppp: Preprocesses C/C++ files with only the specified\n"
	"preprocessor symbols defined and/or undefined.\n\n"
	"Usage: cppp [-DU SYMBOL ...] [SOURCE ... [DEST]]\n"
	"   -D  Defines SYMBOL\n"
	"   -U  Undefines SYMBOL\n\n"
	"If multiple SOURCE files are specified, the last argument\n"
	"(DEST) must be a directory.\n";


static void errmsg(char *szFmt, ...)
{
    va_list	arg;

    va_start(arg, szFmt);
    fputs("error: ", stderr);
    vfprintf(stderr, szFmt, arg);
    fputc('\n', stderr);
}

void *xmalloc(size_t size)
{
    void *p;

    p = malloc(size);
    if (!p) {
	errmsg("Out of memory");
	exit(EXIT_FAILURE);
    }
    return p;
}

void *xrealloc(void *p, size_t size)
{
    p = realloc(p, size);
    if (!p) {
	errmsg("Out of memory");
	exit(EXIT_FAILURE);
    }
    return p;
}

int main(int argc, char *argv[])
{
    FILE       *infile, *outfile;
    errhandler	err;
    strset	defs, undefs;
    ppproc	proc;
    char	cwd[MAX_DIRNAME_SIZE];
    int		n;

    initerrhandler(&err);
    initstrset(&defs);
    initstrset(&undefs);
    for (;;) {
	n = getnextoption(argc, argv, "d:D:u:U:h");
	if (n == ':' || n == '?')
	    return EXIT_FAILURE;
	else if (n == EOF)
	    break;
	else if (n == 'h') {
	    fputs(yowzitch, stdout);
	    return 0;
	} else if (n == 'd' || n == 'D') {
	    addstringtoset(&defs, optionarg());
	    delstringfromset(&undefs, optionarg());
	} else if (n == 'u' || n == 'U') {
	    addstringtoset(&undefs, optionarg());
	    delstringfromset(&defs, optionarg());
	}
    }

    initppproc(&proc, &defs, &undefs, &err);

    n = optionindex();
    if (n >= argc) {
	seterrorfile(&err, NULL);
	prepreprocess(&proc, stdin, stdout);
    } else if (fileisdir(argv[argc - 1])) {
	if (n + 1 == argc) {
	    errmsg("no input files specified.");
	    return EXIT_FAILURE;
	}
	currentdirname(cwd);
	for ( ; n < argc - 1 ; ++n) {
	    seterrorfile(&err, argv[n]);
	    if (!(infile = fopen(argv[n], "r"))) {
		perror(argv[n]);
		continue;
	    }
	    changedir(argv[argc - 1]);
	    outfile = fopen(getfilename(argv[n]), "w");
	    changedir(cwd);
	    if (!outfile) {
		perror(argv[n]);
		fclose(infile);
		continue;
	    }
	    prepreprocess(&proc, infile, outfile);
	    fclose(infile);
	    fclose(outfile);
	}
    } else if (n + 2 < argc) {
	errmsg("\"%s\" is not a directory.", argv[argc - 1]);
	return EXIT_FAILURE;
    } else {
	seterrorfile(&err, argv[n]);
	if (!(infile = fopen(argv[n], "r"))) {
	    perror(argv[n]);
	    return EXIT_FAILURE;
	}
	++n;
	if (n == argc)
	    outfile = stdout;
	else {
	    if (!(outfile = fopen(argv[n], "w"))) {
		perror(argv[n]);
		return EXIT_FAILURE;
	    }
	}
	prepreprocess(&proc, infile, outfile);
	fclose(infile);
	fclose(outfile);
    }

    closeppproc(&proc);
    freestrset(&defs);
    freestrset(&undefs);
    return EXIT_SUCCESS;
}
