#if !defined foo /* ends with Unix newline */
-Ufoo
#else /* ends with MS-DOS newline sequence */
-Dfoo
#endif /* no final newline */