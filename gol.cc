#include <fstream>
#include <array>
#include <assert.h>
#include <list>
#include <threadpool.h>
#include <iostream>
#include "Waiter.h"
#include "DoubleXY.h"
#include "GLstate.h"

// Multiplar av 4, because fu
int SIZEX = 1000;
int SIZEY = 600;

static unsigned int FPSMAX = 60;
static const int NRTHREADS = 64;

static std::mutex fliplock;

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

  void loaddefaults() {
    input(GOSPERGLIDER, 36, 13, 10, 10);
    input(RPENTOMINO, 3, 3, 300, 100);
    input(RPENTOMINO, 3, 3, 100, 300);
    input(BIGGLIDER, 18, 18, 330, 270);
    input(SMALLBOOM, 3, 3, 255, 255);
  }

  void save() { savedstate = aliveactive; }

  void load() { aliveactive = savedstate; }

  bool safe_access(int x, int y) {
    return (x >= 0 && y >= 0 && x < sizex && y < sizey);
  }

  bool aliveat(int x, int y) { return aliveactive[y * sizex + x] == 255; }

  unsigned char action(int x, int y) {
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

  void run() {
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

  void letlive_scaled(DoubleXY &&f, WindowScale const &loc) {
    loc.scaled_to_view(&f);
    letlive(f.int_x(sizex), f.int_y(sizey));
  }

  void letdie_scaled(DoubleXY &&f, WindowScale const &loc) {
    loc.scaled_to_view(&f);
    letdie(f.int_x(sizex), f.int_y(sizey));
  }

  void letlive(int x, int y) {
    if (safe_access(x, y)) {
      aliveactive[y * sizex + x] = 255;
    }
  }

  void letdie(int x, int y) {
    if (safe_access(x, y)) {
      aliveactive[y * sizex + x] = 0;
    }
  }

  void input(const char *input, int width, int height, int xoffs, int yoffs) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++, input++) {
        if (*input == 'O') {
          letlive(x + xoffs, y + yoffs);
        }
      }
    }
  }
};

Board global_b(SIZEX, SIZEY);

std::vector<DoubleXY> line(int x0, int y0, int x1, int y1, int sizex, int sizey) {
  std::vector<DoubleXY> ret;

  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int err2;

  while (true) {
    ret.emplace_back(x0, y0, sizex, sizey);
    if (x0 == x1 && y0 == y1)
      break;
    err2 = err;
    if (err2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (err2 < dy) {
      err += dx;
      y0 += sy;
    }
  }

  return ret;
}

int main(int argc, const char *argv[]) {
  {
    if (argc == 3) {
      SIZEX = atoi(argv[1]);
      SIZEY = atoi(argv[2]);
      if (!SIZEX || !SIZEY) {
        std::cout << "Incorrect input parameters, atoi didn't work on: '"
                  << argv[1] << "', '" << argv[2] << "'" << std::endl;
        return 1;
      }
    }
  }

  GLstate state;


  bool active = 1;
  bool running = 1;
  std::thread boardthread([&active, &running]() {
    Waiter runwaiter(1000 / FPSMAX);
    while (running) {
      if (active) {
        std::unique_lock<std::mutex> f(fliplock);
        global_b.run();
      }
      runwaiter.set_ms_tick_length(1000 / FPSMAX);
      runwaiter.wait_if_fast();
    }
  });

  int lastx = 0, lasty = 0;
  bool lbdown = 0;
  bool rbdown = 0;
  int windowsizex = SIZEX, windowsizey = SIZEY;
  SDL_Event event;
  Waiter waiter(16);
  while (true) {
    {
      std::unique_lock<std::mutex> f(fliplock);
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
          running = 0;
          boardthread.join();
          return 0;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
          case SDLK_q:
            running = 0;
            boardthread.join();
            return 0;
          case SDLK_SPACE:
            active = !active;
            break;
          case SDLK_i:
            if (FPSMAX > 2 && FPSMAX <= 10) {
              FPSMAX--;
            } else if (FPSMAX > 10) {
              FPSMAX = FPSMAX * 8 / 10;
            }
            break;
          case SDLK_o:
            if (FPSMAX <= 10) {
              FPSMAX++;
            } else if (FPSMAX > 10) {
              FPSMAX = FPSMAX * 12 / 10;
            }
            break;
          case SDLK_d:
            for (auto &i : global_b.aliveactive) {
              i = 0;
            }
            break;
          case SDLK_r:
            global_b.loaddefaults();
            break;
          case SDLK_f:
            {
              std::ofstream logfile;
              logfile.open("log");
              if (!logfile) {
                assert(0 && "logfile error");
                return 0;
              }
              for (int y = 0; y < SIZEY; y++) {
                for (int x = 0; x < SIZEX; x++) {
                  logfile << (global_b.aliveactive[y * SIZEX + x] == 255 ? 'O'
                      : ' ');
                }
                logfile << std::endl;
              }
            }
            break;
          case SDLK_s:
            global_b.save();
            break;
          case SDLK_l:
            global_b.load();
            break;
          case SDLK_PERIOD:
            global_b.run();
            break;
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
          lastx = event.button.x;
          lasty = event.button.y;
          switch (event.button.button) {
            case SDL_BUTTON_LEFT:
              global_b.letlive_scaled(DoubleXY(lastx, lasty, windowsizex, windowsizey), state.loc);
              lbdown = 1;
              break;
            case SDL_BUTTON_RIGHT:
              global_b.letdie_scaled(DoubleXY(lastx, lasty, windowsizex, windowsizey), state.loc);
              rbdown = 1;
              break;
          }
          break;
        case SDL_MOUSEBUTTONUP:
          switch (event.button.button) {
            case SDL_BUTTON_LEFT:
              lbdown = 0;
              break;
            case SDL_BUTTON_RIGHT:
              rbdown = 0;
              break;
          }
          break;
        case SDL_MOUSEMOTION:
          if (lbdown || rbdown) {
            int currx = event.motion.x;
            int curry = event.motion.y;
            auto xys = line(lastx, lasty, currx, curry, windowsizex, windowsizey);
            lastx = currx;
            lasty = curry;

            for (DoubleXY xy : xys) {
              if (lbdown) {
                global_b.letlive_scaled(std::move(xy), state.loc);
              } else if (rbdown) {
                global_b.letdie_scaled(std::move(xy), state.loc);
              }
            }
          }
          break;
        case SDL_MOUSEWHEEL:
          {
            int x, y;
            SDL_GetMouseState(&x, &y);
            DoubleXY f(x, y, windowsizex, windowsizey);

            if (event.wheel.y > 0) {
              state.zoomin(f.x, f.y);
            } else {
              state.zoomout(f.x, f.y);
            }
          }
          break;
        case SDL_WINDOWEVENT:
          if (event.window.event ==  SDL_WINDOWEVENT_RESIZED) {
            windowsizex = event.window.data1;
            windowsizey = event.window.data2;
          }
          break;
        }
      }
    }

    waiter.wait_if_fast();

    // draw allways
    state.draw(global_b.aliveactive.data(), SIZEX, SIZEY);
  }

  return 0;
}
