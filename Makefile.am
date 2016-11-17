AUTOMAKE_OPTIONS = foreign
SUBDIRS = src doc scripts tests

clean-local:
	rm -f doc/*.1

distclean-local:
	rm -rf autom4te.cache
	rm -f aclocal.m4 config.h.* configure Makefile.in
	rm -f doc/Makefile.in scripts/Makefile.in src/Makefile.in \
              tests/Makefile.in
	rm -f support/compile support/depcomp support/install-sh \
              support/missing support/test-driver
