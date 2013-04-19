/* cppp.c: Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include "gen.h"
#include "unixisms.h"
#include "error.h"
#include "symset.h"
#include "ppproc.h"

/* Online help text.
 */
static char const *const yowzitch =
    "Usage: cppp [OPTIONS] [SOURCE ... [DEST]]\n"
    "Partially preprocesses C/C++ files, with only specific preprocessor\n"
    "symbols being defined (and/or undefined).\n\n"
    "      -D SYMBOL[=VALUE]   Preprocess SYMBOL as defined (to VALUE).\n"
    "      -U SYMBOL           Preprocess SYMBOL as undefined.\n"
    "      --help              Display this help and exit.\n"
    "      --version           Display version information and exit.\n\n"
    "If DEST is omitted, the resulting source is emitted to standard output.\n"
    "If multiple SOURCE files are specified, the last argument DEST must be\n"
    "a directory.\n";

/* Version identifier.
 */
static char const *const vourzhon =
    "cppp: version 2.2\n"
    "Copyright (C) 2011 by Brian Raiter <breadbox@muppetlabs.com>\n"
    "License GPLv2+: GNU GPL version 2 or later.\n"
    "This is free software; you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n";

/* Display an error message regarding command-line syntax and exit.
 */
static void fail(char const *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fputs("cppp: ", stderr);
    vfprintf(stderr, fmt, args);
    fputs("\nTry \"cppp --help\" for more information.\n", stderr);
    exit(EXIT_FAILURE);
}

/* Parse the command-line options, storing the specified symbols to
 * define and/or undefine in defs and undefs. The arguments specifying
 * the input/output files are left in argv. The return value is the
 * new value for argc.
 */
static int readcmdline(int argc, char *argv[],
		       struct symset *defs,
		       struct symset *undefs)
{
    char *arg, *p;
    long value;
    int i, j;

    for (i = j = 1 ; i < argc ; ++i) {
	if (argv[i][0] != '-') {
	    argv[j++] = argv[i];
	    continue;
	}
	if (!strcmp(argv[i], "--help")) {
	    fputs(yowzitch, stdout);
	    exit(EXIT_SUCCESS);
	} else if (!strcmp(argv[i], "--version")) {
	    fputs(vourzhon, stdout);
	    exit(EXIT_SUCCESS);
	}
	if (!strcmp(argv[i], "--")) {
	    for (++i ; i < argc ; ++i)
		argv[j++] = argv[i];
	    break;
	}
	if (argv[i][1] == 'D') {
	    arg = argv[i] + 2;
	    if (!*arg) {
		if (i + 1 < argc)
		    arg = argv[++i];
		else
		    fail("missing argument to -D");
	    }
	    p = strchr(arg, '=');
	    if (p) {
		*p++ = '\0';
		if (!*p)
		    fail("missing value for symbol %s", arg);
		value = strtol(p, &p, 0);
		if (*p || value == LONG_MIN || value == LONG_MAX)
		    fail("invalid numeric value for symbol %s", arg);
	    } else {
		value = 1;
	    }
	    if (findsymbolinset(undefs, arg, NULL))
		fail("symbol %s cannot be both defined and undefined", arg);
	    addsymboltoset(defs, arg, value);
	} else if (argv[i][1] == 'U') {
	    arg = argv[i] + 2;
	    if (!*arg) {
		if (i + 1 < argc)
		    arg = argv[++i];
		else
		    fail("missing argument to -U");
	    }
	    if (findsymbolinset(defs, arg, NULL))
		fail("symbol %s cannot be both defined and undefined", arg);
	    addsymboltoset(undefs, arg, 0L);
	} else {
	    fail("invalid option: %s", argv[i]);
	}
    }
    return j;
}

/* Run the partial preprocessor. The details of the input and output
 * depend on the number of command-line arguments. With no arguments,
 * standard input is processed to standard output. With one argument,
 * the file is processed to standard output. With two file arguments,
 * the first file is processed to the second file. With one or more
 * file arguments followed by a directory argument, the files are
 * processed to files with the same name in the given directory.
 */
int main(int argc, char *argv[])
{
    FILE *infile, *outfile;
    char const *filename, *dirname;
    struct symset *defs, *undefs;
    struct ppproc *ppp;
    int exitcode;
    int i;

    defs = initsymset();    
    undefs = initsymset();    

    argc = readcmdline(argc, argv, defs, undefs);

    ppp = initppproc(defs, undefs);

    exitcode = EXIT_SUCCESS;
    if (argc <= 1) {
	seterrorfile(NULL);
	partialpreprocess(ppp, stdin, stdout);
    } else if (argc == 2) {
	filename = argv[1];
	seterrorfile(filename);
	if (!(infile = fopen(filename, "r"))) {
	    perror(filename);
	    return EXIT_FAILURE;
	}
	partialpreprocess(ppp, infile, stdout);
	fclose(infile);
    } else if (fileisdir(argv[argc - 1])) {
	dirname = argv[argc - 1];
	for (i = 1 ; i < argc - 1 ; ++i) {
	    filename = argv[i];
	    seterrorfile(filename);
	    if (!(infile = fopen(filename, "r"))) {
		perror(filename);
		exitcode = EXIT_FAILURE;
		continue;
	    }
	    savedir();
	    if (!changedir(dirname)) {
		perror(dirname);
		exit(EXIT_FAILURE);
	    }
	    filename = getbasefilename(filename);
	    outfile = fopen(filename, "w");
	    if (outfile) {
		partialpreprocess(ppp, infile, outfile);
		if (fclose(outfile)) {
		    perror(filename);
		    exitcode = EXIT_FAILURE;
		}
	    } else {
		perror(filename);
		exitcode = EXIT_FAILURE;
	    }
	    restoredir();
	    fclose(infile);
	}
    } else if (argc == 3) {
	filename = argv[1];
	seterrorfile(filename);
	if (!(infile = fopen(filename, "r"))) {
	    perror(filename);
	    return EXIT_FAILURE;
	}
	filename = argv[2];
	if (!(outfile = fopen(filename, "w"))) {
	    perror(filename);
	    return EXIT_FAILURE;
	}
	partialpreprocess(ppp, infile, outfile);
	fclose(infile);
	if (fclose(outfile)) {
	    perror(filename);
	    exitcode = EXIT_FAILURE;
	}
    } else {
	fail("\"%s\" is not a directory.", argv[argc - 1]);
    }

    freeppproc(ppp);
    freesymset(defs);
    freesymset(undefs);
    return exitcode;
}
