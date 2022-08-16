#ifdef foo
-Dfoo
#else
#if !defined(foo)
-Ufoo
#endif
#endif
