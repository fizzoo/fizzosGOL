#ifndef BOARD_H
#define BOARD_H

#include <threadpool.h>
#include "DoubleXY.h"
#include "WindowScale.h"

static const int NRTHREADS = 64;

struct Board {
  int sizex, sizey;
  Board(int maxx, int maxy) : sizex(maxx), sizey(maxy) {}

  std::vector<unsigned char> aliveactive =
      std::vector<unsigned char>(sizex * sizey, 0);
  std::vector<unsigned char> alivewait =
      std::vector<unsigned char>(sizex * sizey, 0);
  std::vector<unsigned char> savedstate =
      std::vector<unsigned char>(sizex * sizey, 0);

  ThreadPool pool;

  void loaddefaults();
  void save();
  void load();
  bool safe_access(int x, int y);
  bool aliveat(int x, int y);
  unsigned char action(int x, int y);
  void run();
  void letlive_scaled(DoubleXY &&f, WindowScale const &loc);
  void letdie_scaled(DoubleXY &&f, WindowScale const &loc);
  void letlive(int x, int y);
  void letdie(int x, int y);
  void input(const char *input, int width, int height, int xoffs, int yoffs);
};

#endif /* end of include guard: BOARD_H */
