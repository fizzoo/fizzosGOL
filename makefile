CXXFLAGS= -Wall -Wextra -pedantic -std=c++14
OPT=-march=native -O3 -DNDEBUG
DEFS=

ifeq ($(shell uname), Linux)
LDFLAGS=-pthread -lSDL2 -lGLEW -lGL -lGLU
else
LDFLAGS=-lSDL2 -lSDL2main -lGLEW32 -lOpenGL32 -lGLU32
endif

SRC=gol.cc Waiter.cc ../threadpool/threadpool.cc

default: gol

opt: $(SRC)
	g++ $(OPT) -o gol $^ $(CXXFLAGS) $(LDFLAGS) $(DEFS)

gol: $(SRC)
	g++ -Og -g -o gol $^ $(CXXFLAGS) $(LDFLAGS) $(DEFS)

clean:
	rm -f gol.exe gol
