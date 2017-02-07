ifdef DEBUG
CPPFLAGS = -fPIC -ggdb -Wall -DDEBUG -std=c++11
else
CPPFLAGS = -fPIC -g -Wall -O3 -minline-all-stringops -std=c++11
endif

ifdef PROFILE
CPPFLAGS += -pg
endif

CPPFLAGS += -DUSE_SGI_ROPE

FLEX = `which flex35 2>/dev/null || which flex 2>/dev/null`

all: libxhp.a

clean:
	-rm libxhp.a xhpize parser.yacc.cpp scanner.lex.cpp scanner.lex.hpp parser.yacc.output parser.yacc.hpp fastpath.cpp version.h *.o 2>/dev/null

parser.yacc.cpp: parser.y
	bison --debug --verbose -d -o $@ $<

parser.yacc.hpp: parser.yacc.cpp

scanner.lex.cpp: scanner.l
	$(FLEX) \
	  -C --header-file=scanner.lex.hpp -o $@ -d $<

scanner.lex.hpp: scanner.lex.cpp

fastpath.cpp: fastpath.re
	re2c -c -o $@ $<

%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

xhp_preprocess.o: xhp_preprocess.cpp scanner.lex.hpp parser.yacc.hpp
parser.yacc.o: scanner.lex.hpp
scanner.lex.o: parser.yacc.hpp

libxhp.a: code_rope.o rope.o rope_entity.o scanner.lex.o parser.yacc.o fastpath.o xhp_preprocess.o
	$(AR) -crs $@ $^

version.h:
	echo "#define XHPIZE_VERSION \""`git show -s --format="%h - %ci" HEAD`"\"" >> version.h

xhpize: xhpize.cpp libxhp.a version.h
	g++ $(CPPFLAGS) -o $@ $^
# xhpize: xhpize.cpp libxhp.a version.h
#	g++ $(CPPFLAGS) -L. -lxhp -o $@ $<

rope_test.exe: rope_test.cpp code_rope.o rope.o rope_entity.o
	g++ $(CPPFLAGS) -o $@ $^

rope_test: rope_test.exe rope_test.expect
	./rope_test.exe | diff rope_test.expect -

.PHONY: all clean tags
