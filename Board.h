#ifndef BOARD_H
#define BOARD_H

#include "DoubleXY.h"
#include "WindowScale.h"
#include <threadpool.h>

static const int NRTHREADS = 64;

/**
 * Contains the grid, logic for the actual game, and ways to interact with the
 * grid.
 */
class Board {
private:
  int sizex, sizey;

  // Two equivalent data-grids in order to enable double buffering, plus one
  // for saving states temporarily
  std::vector<unsigned char> aliveactive =
      std::vector<unsigned char>(sizex * sizey, 0);
  std::vector<unsigned char> alivewait =
      std::vector<unsigned char>(sizex * sizey, 0);
  std::vector<unsigned char> savedstate =
      std::vector<unsigned char>(sizex * sizey, 0);

  ThreadPool pool;

  bool safe_access(int x, int y);
  void input(const char *input, int width, int height, int xoffs, int yoffs);
  unsigned char action(int x, int y);

public:
  Board(int maxx, int maxy) : sizex(maxx), sizey(maxy) {}

  /**
   * Enter an example input with a few gliders and such.
   */
  void loaddefaults();

  /**
   * Saves and loads to/from savedstate
   */
  void save();
  void load();

  /**
   * Functions for changing or checking state at a specific position.
   */
  bool aliveat(int x, int y);
  void letlive_scaled(DoubleXY &&f, WindowScale const &loc);
  void letdie_scaled(DoubleXY &&f, WindowScale const &loc);
  void letlive(int x, int y);
  void letdie(int x, int y);

  /**
   * Runs an iteration of GOL, swapping buffers.
   */
  void run();

  void clear();

  /**
   * Returns the data of the currently active buffer, for example for drawing.
   */
  unsigned char *data();
};

#endif /* end of include guard: BOARD_H */
