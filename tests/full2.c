#if (defined(foo) || defined bar)
one
#elif (!defined(foo) && !defined(baz))  // comment
two
#elif defined foo ? !defined bar : !defined baz
three
#endif
