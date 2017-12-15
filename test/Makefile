FLAGS=-O0 -g -Wall -I../include
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=c++11
OBJS=RetroAchievements.o ../src/dejson.o Main.o

%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

all: test

test: $(OBJS)
	g++ -o $@ $+

RetroAchievements.c: RetroAchievements.dej RetroAchievements.h
	../../ddlt/ddlt ../compiler/dejson.lua -c $<

RetroAchievements.h: RetroAchievements.dej
	../../ddlt/ddlt ../compiler/dejson.lua -h $<

clean:
	rm -f test $(OBJS) RetroAchievements.h RetroAchievements.c
