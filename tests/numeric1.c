#if foo
one
#elif bar
two
#endif
#if foo > bar
three
#elif foo < bar
four
#else
five
#endif
#if foo + bar == foo * bar + 1
six
#endif
