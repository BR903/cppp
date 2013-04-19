#include	<unistd.h>
#include	<getopt.h>
#include	<sys/stat.h>
#include	"unixisms.h"

void currentdirname(char *name)
{
    getcwd(name, MAX_DIRNAME_SIZE);
}

void changedir(char const *name)
{
    chdir(name);
}

int getnextoption(int argc, char *argv[], char const *options)
{
    return getopt(argc, argv, options);
}

char *optionarg(void)
{
    return optarg;
}

int optionindex(void)
{
    return optind;
}

int fileisdir(char const *name)
{
    struct stat s;

    if (stat(name, &s))
	return 0;
    return S_ISDIR(s.st_mode);
}

char *getfilename(char const *name)
{
    char const *r, *s;

    r = name;
    for (s = name + 1 ; *s ; ++s)
	if (s[-1] == '/')
	    r = s;
    return (char*)r;
}
