#ifdef foo
/*1*/
#ifndef bar
/*2*/
#endif
#else
#ifndef baz
/*3*/
#endif
#endif
#if (defined(foo) || defined(bar))
/*4*/
#elif (!defined(foo) && !defined(baz))  // comment
/*5*/
#elif defined foo ? !defined bar : !defined baz
/*6*/
#endif
