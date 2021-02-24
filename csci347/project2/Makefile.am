bin_PROGRAMS=ls find

ls_SOURCES=ls.c list.c text_io.c
find_SOURCES=find.c list.c text_io.c

test_scripts=tests/ls_order  \
			 tests/ls_exists \
			 tests/ls_dash_a  \
			 tests/find_exists \
			 tests/find_type \
			 tests/find_exec

EXTRA_DIST=${test_scripts} list.h text_io.h
TESTS=${test_scripts}

#XFAIL_TESTS=tests/ls_order
