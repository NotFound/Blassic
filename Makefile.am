# Makefile.am
# Originally written by Pierre Sarrazin
# Changes and additions by Julián Albo
# Last revision 23-feb-2012

noinst_PROGRAMS = gencharset
bin_PROGRAMS = blassic


# This tells Automake that charset.cpp must be generated before
# anything else is compiled.

# Changed, now use several charsets.
#BUILT_SOURCES = charset.cpp
# Testing other way.
#BUILT_SOURCES = charset_default.cpp charset_cpc.cpp charset_spectrum.cpp charset_msx.cpp
BUILT_SOURCES = gencharset$(EXEEXT)

# Needed to generate charset cpp files:

gencharset_SOURCES = gencharset.cpp

gencharset_CXXFLAGS = $(CXXFLAGS_FOR_BUILD)

gencharset_LDFLAGS =

# This way the .exe extension is appended when compiling for windows
# under linux, but it works anyway.
gencharset$(EXEEXT): gencharset.cpp
	$(CXX_FOR_BUILD) $(gencharset_CXXFLAGS) $(gencharset_LDFLAGS) \
		-o gencharset$(EXEEXT) \
		gencharset.cpp

# Character sets for graphics modes.

#charset.cpp: $(srcdir)/charset.def gencharset
#	./gencharset $(srcdir)/charset.def charset.cpp
#	test -f charset.cpp

charset_default.cpp: $(srcdir)/default.def gencharset.cpp charset.h
	./gencharset$(EXEEXT) $(srcdir)/default.def \
		charset_default.cpp default
	test -f charset_default.cpp

charset_cpc.cpp: $(srcdir)/cpc.def gencharset.cpp charset.h
	./gencharset$(EXEEXT) $(srcdir)/cpc.def \
		charset_cpc.cpp cpc
	test -f charset_cpc.cpp

charset_spectrum.cpp: $(srcdir)/spectrum.def gencharset.cpp charset.h
	./gencharset$(EXEEXT) $(srcdir)/spectrum.def \
		charset_spectrum.cpp spectrum
	test -f charset_spectrum.cpp

charset_msx.cpp: $(srcdir)/msx.def gencharset.cpp charset.h
	./gencharset$(EXEEXT) $(srcdir)/msx.def \
		charset_msx.cpp msx
	test -f charset_msx.cpp


blassic_SOURCES = \
	charset_default.cpp \
	charset_cpc.cpp \
	charset_spectrum.cpp \
	charset_msx.cpp \
	charset.h \
	blassic.cpp \
	blassic.h \
	codeline.cpp \
	codeline.h \
	cursor.cpp \
	cursor.h \
	dim.cpp \
	dim.h \
	directory.cpp \
	directory.h \
	dynamic.cpp \
	dynamic.h \
	edit.cpp \
	edit.h \
	element.h \
	element.cpp \
	error.cpp \
	error.h \
	file.cpp \
	file.h \
	fileconsole.cpp \
	filepopen.cpp \
	fileprinter.cpp \
	filesocket.cpp \
	filewindow.cpp \
	function.cpp \
	function.h \
	graphics.cpp \
	graphics.h \
	key.cpp \
	key.h \
	keyword.cpp \
	keyword.h \
	mbf.cpp \
	mbf.h \
	memory.cpp \
	memory.h \
	program.cpp \
	program.h \
	regexp.cpp \
	regexp.h \
	result.h \
	runner.cpp \
	runner.h \
	runnerline.cpp \
	runnerline.h \
	runnerline_impl.cpp \
	runnerline_impl.h \
	runnerline_instructions.cpp \
	runnerline_print.cpp \
	showerror.cpp \
	showerror.h \
	socket.cpp \
	socket.h \
	sysvar.cpp \
	sysvar.h \
	token.cpp \
	token.h \
	trace.cpp \
	trace.h \
	using.cpp \
	using.h \
	util.h \
	var.cpp \
	var.h \
	version.cpp

blassic_CXXFLAGS = @SVGALIB_CFLAGS@ @BL_X_CFLAGS@
blassic_LDFLAGS = @BL_X_LIBS@
blassic_LDADD = @SVGALIB_LIBS@ @CYGWIN_FLAGS@ @BL_X_ADD@


#	testdl: a tiny library to test blassic dynamic link.


# Julian: modified this to allow cross-compiling.
#testdl.so: testdl.o
#	gcc -shared -Wl,-soname,testdl.so -o testdl.so testdl.o
#testdl.o: testdl.cpp
#	gcc -Wall -fPIC -c testdl.cpp

#testdl_CXXFLAGS = \
#	-W -Wall -Wwrite-strings -Wstrict-prototypes \
#	-Wunused

testdl.so: testdl.o
	$(CXX) -shared -Wl,-soname,testdl.so $(LDFLAGS) -o testdl.so testdl.o

testdl.o: testdl.cpp
	$(CXX) -fPIC $(testdl_CXXFLAGS) -c testdl.cpp


rpm: dist
	rpm -ta $(distdir).tar.gz

EXTRA_DIST = \
	bootstrap autogen.sh \
	do_conf \
	do_confnodeb \
	do_confcross_arm \
	do_confcross_mingw \
	do_confhpux \
	blassic.spec \
	alphabet.blc automod.blc \
	blassic.bpr \
	testdl.bpr \
	random.dat \
	gencharset.cpp \
	charset_default.cpp charset_cpc.cpp charset_spectrum.cpp charset_msx.cpp \
	default.def cpc.def spectrum.def msx.def \
	counter.sh \
	testdl.cpp

#CLEANFILES = testdl.so $(BUILT_SOURCES)
CLEANFILES = testdl.so

# When Automake needs to regenerate configure, the following options
# will be passed to aclocal, as in the bootstrap script.
ACLOCAL_AMFLAGS = -I .


# Boilerplate:

auxdir = @ac_aux_dir@
AUX_DIST = $(auxdir)/install-sh $(auxdir)/missing $(auxdir)/mkinstalldirs

MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	configure \
	sic/config-h.in \
	sic/stamp-h.in \
	$(AUX_DIST) \
	depcomp \
	config.guess \
	config.log \
	config.status \
	config.sub \
	install-sh \
	missing \
	mkinstalldirs

