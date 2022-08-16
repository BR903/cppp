??=if defined foo /* trigraph for # */
-Dfoo
??=elif ??-foo == ??-0 /* more trigraphs */
-Ufoo
??=??/
else /* backslash-newline pair with trigraph */
failed -Ufoo test
??=??/
e??/
n??/
d??/
i??/
f /* backslash-newline pair with trigraph and MS-DOS newline */
