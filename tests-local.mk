# This file is loaded by tests/Makefile.am.  It contains things that
# are specific to a given project or even Svn branch.
UCONSOLE_CHECK_FLAGS = -k2

TESTS_DIRS = 0.x 1.x 2.x uob

URBI_SERVER = $(abs_top_builddir)/src/urbi-console
# Run k2 tests only.
k2-check:
	$(MAKE) check TESTS_DIRS=2.x

# k2 is under heavy development, don't catch SEGV and the like as hard errors
# while running make check.
ENABLE_HARD_ERRORS = false

# pending features
TFAIL_TESTS +=					\
2.x/onevent.chk					\
2.x/string/escape.chk

# test we don't want to care about temporarily
TFAIL_TESTS +=					\
2.x/derive.chk

# k1 tests that currently don't pass, but we should.
# In fact, the above list has been removed to gain some time
# when running tests. The following tests should be examined
# and reintroduced in 1.x as needed.
TO_CHECK_TESTS =				\
# +begin, +end                                  \
1.-/begin-end-report.chk			\
# chan << echo(foo) goes in chan                \
1.-/channels-tags.chk				\
# +connection                                   \
1.-/connection-delete.chk			\
# emit foo                                      \
1.-/events.chk					\
1.-/events-emit-then-return-in-function.chk	\
1.-/every-emit.chk				\
# []-access to list and hash                    \
1.-/hash-and-list.chk				\
# events                                        \
1.-/lazy-test-eval.chk				\
1.-/loadwav.chk					\
1.-/object-events.chk

# Uobject tests that we fail because features are not implemented
XFAIL_TESTS +=					\
2.x/uob/group.chk				\
uob/remote/all-write-prop.chk
