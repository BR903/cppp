#ifdef foo
-Dfoo
#else
#if !defined(foo)
-Ufoo
#endif
#endif
$
/*1*/#/*2*/ifdef/*3*/foo\
/*4*/
-Dfoo
  # else //\
5
-Ufoo
# \
    endif
