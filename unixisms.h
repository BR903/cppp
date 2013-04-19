#ifndef	_unixisms_h_
#define	_unixisms_h_

#define	MAX_DIRNAME_SIZE	256

extern void currentdirname(char *name);
extern void changedir(char const *name);
extern int getnextoption(int argc, char *argv[], char const *options);
extern char *optionarg(void);
extern int optionindex(void);
extern int fileisdir(char const *name);
extern char *getfilename(char const *name);

#endif
