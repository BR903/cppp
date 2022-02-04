#ifdef foo
-Dfoo
#else
-Ufoo
#endif
#endif
$
#if
empty
#endif
$
#ifdef foo
defined
#elif foo < 0
negative
#endif
$
#if foo > 0
positive
#elifndef foo
undefined
#endif
$
#ifdef foo
-Dfoo
#endif
-Ufoo
#else
$
#elif defined foo
-Dfoo
#else
-Ufoo
#endif
$
#elifdef foo
-Dfoo
#else
-Ufoo
#endif
