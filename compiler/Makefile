ifeq ($(shell uname -a),)
	EXE=.exe
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	EXE=.exe
else
	EXE=
endif

CXXFLAGS=-O0 -g -Wall -std=c++11 -I../include
OBJS=Generator.o GeneratorC.o GeneratorH.o Lexer.o Main.o Parser.o

%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

all: dejson$(EXE)

dejson$(EXE): $(OBJS)
	g++ -o $@ $+

Lexer.o: Lexer.cpp Tokens.h

Tokens.h: Tokens.gperf
	gperf -c -C -l -L C++ -t -7 -m 100 -I --output-file=$@ $<

clean:
	rm -f dejson$(EXE) $(OBJS) Tokens.h
