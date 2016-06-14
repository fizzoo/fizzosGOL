CXXFLAGS= -std=c++14 -march=native -O3 -DNDEBUG

ifeq ($(shell uname), Linux)
LDFLAGS=-pthread -lSDL2 -lGLEW -lGL -lGLU
else
LDFLAGS=-lSDL2 -lSDL2main -lGLEW32 -lOpenGL32 -lGLU32
endif

SRCS=gol.cc Waiter.cc GLstate.cc WindowScale.cc Board.cc
OBJS=$(subst .cc,.o,$(SRCS))

gol: $(OBJS)
	g++ -o gol $(OBJS) $(CXXFLAGS) $(LDFLAGS)

debug: CXXFLAGS += -Wall -Wextra -pedantic -Og -g -DDEBUG
debug: clean gol

Board.o: Board.cc DoubleXY.h WindowScale.h
GLstate.o: GLstate.cc WindowScale.h
WindowScale.o: WindowScale.cc DoubleXY.h

install: gol
	cp gol /usr/local/bin/

clean:
	rm -f gol.exe gol *.o
