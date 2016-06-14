#include "Board.h"

static const char *GOSPERGLIDER = "...................................."
                                  "...................................."
                                  "...................................."
                                  "...................................."
                                  ".......................O.O.........."
                                  ".....................O...O.........."
                                  ".............O.......O.............."
                                  "............OOOO....O....O........OO"
                                  "...........OO.O.O....O............OO"
                                  "OO........OOO.O..O...O...O.........."
                                  "OO.........OO.O.O......O.O.........."
                                  "............OOOO...................."
                                  ".............O......................";

static const char *BIGGLIDER = "   OOO            "
                               "   O  OOO         "
                               "    O O           "
                               "OO       O        "
                               "O O    O  O       "
                               "O        OO       "
                               " OO               "
                               " O  O     O OO    "
                               " O         OO O   "
                               "   O O      OO  O "
                               "    OO O    OO   O"
                               "        O       O "
                               "       OOOO   O O "
                               "       O OO   OOOO"
                               "        O   OO O  "
                               "             OO   "
                               "         O OOO    "
                               "          O  O    ";

static const char *RPENTOMINO = " OO"
                                "OO "
                                " O ";

static const char *SMALLBOOM = "OOO"
                               "O O"
                               "O O";

void Board::loaddefaults() {
  input(GOSPERGLIDER, 36, 13, 10, 10);
  input(RPENTOMINO, 3, 3, 300, 100);
  input(RPENTOMINO, 3, 3, 100, 300);
  input(BIGGLIDER, 18, 18, 330, 270);
  input(SMALLBOOM, 3, 3, 255, 255);
}

void Board::save() { savedstate = aliveactive; }

void Board::load() { aliveactive = savedstate; }

bool Board::safe_access(int x, int y) {
  return (x >= 0 && y >= 0 && x < sizex && y < sizey);
}

bool Board::aliveat(int x, int y) { return aliveactive[y * sizex + x] == 255; }

unsigned char Board::action(int x, int y) {
  if (x <= 0 || y <= 0 || x >= sizex - 1 || y >= sizey - 1) {
    return 0;
  }

  int around = 0;
  around += aliveat(x - 1, y - 1);
  around += aliveat(x, y - 1);
  around += aliveat(x + 1, y - 1);
  around += aliveat(x - 1, y);
  around += aliveat(x + 1, y);
  around += aliveat(x - 1, y + 1);
  around += aliveat(x, y + 1);
  around += aliveat(x + 1, y + 1);

  if (aliveat(x, y)) {
    return around == 2 || around == 3 ? 255 : 0x7F;
  } else {
    if (around == 3) {
      return 255;
    } else {
      return aliveactive[y * sizex + x] * 0.97;
    }
  }
}

void Board::run() {
  int each = sizey / NRTHREADS;
  for (int i = 0; i < NRTHREADS; ++i) {
    pool([=]() {
      for (int y = i * each;
           y < (i + 1) * each || ((i + 1) == NRTHREADS && y < sizey); y++) {
        for (int x = 0; x < sizex; x++) {
          alivewait[y * sizex + x] = action(x, y);
        }
      }
    });
  }
  pool.wait_until_done();

  swap(aliveactive, alivewait);
}

unsigned char *Board::data() { return aliveactive.data(); }

void Board::clear() {
  for (auto &i : aliveactive) {
    i = 0;
  }
}

void Board::letlive_scaled(DoubleXY &&f, WindowScale const &loc) {
  loc.scaled_to_view(&f);
  letlive(f.int_x(sizex), f.int_y(sizey));
}

void Board::letdie_scaled(DoubleXY &&f, WindowScale const &loc) {
  loc.scaled_to_view(&f);
  letdie(f.int_x(sizex), f.int_y(sizey));
}

void Board::letlive(int x, int y) {
  if (safe_access(x, y)) {
    aliveactive[y * sizex + x] = 255;
  }
}

void Board::letdie(int x, int y) {
  if (safe_access(x, y)) {
    aliveactive[y * sizex + x] = 0;
  }
}

void Board::input(const char *input, int width, int height, int xoffs,
                  int yoffs) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++, input++) {
      if (*input == 'O') {
        letlive(x + xoffs, y + yoffs);
      }
    }
  }
}
