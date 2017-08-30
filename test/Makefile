ifeq ($(shell uname -a),)
	EXE=.exe
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	EXE=.exe
else
	EXE=
endif

FLAGS=-O0 -g -Wall -I../include
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=c++11
OBJS=RetroAchievements.o ../src/dejson.o Main.o

%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

all: test$(EXE)

test$(EXE): $(OBJS)
	g++ -o $@ $+

RetroAchievements.c: RetroAchievements.dej
	../compiler/dejson --output RetroAchievements $<

RetroAchievements.h: RetroAchievements.dej
	../compiler/dejson --output RetroAchievements $<

clean:
	rm -f test$(EXE) $(OBJS) RetroAchievements.h RetroAchievements.c
