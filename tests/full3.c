#ifdef foo
one
#elifdef bar
two
#elifndef baz
three
#endif
#ifdef foo
four
#ifndef bar
five
#elifdef baz
six
#endif
#endif
