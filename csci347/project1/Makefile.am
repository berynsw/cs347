bin_PROGRAMS=ls
ls_SOURCE=ls.c list.c list.h

test_scripts=tests/ls_order  \
			 tests/ls_exists \
			 tests/ls_dasha

EXTRA_DIST=${test_scripts} list.h list.c
TESTS=${test_scripts}

#XFAIL_TESTS=tests/ls_order
