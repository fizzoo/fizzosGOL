#include "Board.h"
#include "DoubleXY.h"
#include "GLstate.h"
#include "Waiter.h"
#include <array>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <list>

// Multiplar av 4, because fu
int SIZEX = 1920;
int SIZEY = 1080;

static unsigned int FPSMAX = 60;

static std::mutex fliplock;

std::vector<DoubleXY> line(int x0, int y0, int x1, int y1, int winsizex,
                           int winsizey) {
  // Convert from window-pixels to grid
  DoubleXY xy(x0, y0, winsizex, winsizey);
  x0 = xy.int_x(SIZEX);
  y0 = xy.int_y(SIZEY);
  xy = DoubleXY(x1, y1, winsizex, winsizey);
  x1 = xy.int_x(SIZEX);
  y1 = xy.int_y(SIZEY);

  std::vector<DoubleXY> ret;

  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int err2;

  while (true) {
    ret.emplace_back(x0, y0, SIZEX, SIZEY);
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

  Board board(SIZEX, SIZEY);
  GLstate state;

  bool active = 1;
  bool running = 1;
  std::thread boardthread([&board, &active, &running]() {
    Waiter runwaiter(1000 / FPSMAX);
    while (running) {
      if (active) {
        std::unique_lock<std::mutex> f(fliplock);
        board.run();
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
            for (auto &i : board.aliveactive) {
              i = 0;
            }
            break;
          case SDLK_r:
            board.loaddefaults();
            break;
          case SDLK_f: {
            std::ofstream logfile;
            logfile.open("log");
            if (!logfile) {
              assert(0 && "logfile error");
              return 0;
            }
            for (int y = 0; y < SIZEY; y++) {
              for (int x = 0; x < SIZEX; x++) {
                logfile << (board.aliveactive[y * SIZEX + x] == 255 ? 'O'
                                                                       : ' ');
              }
              logfile << std::endl;
            }
          } break;
          case SDLK_s:
            board.save();
            break;
          case SDLK_l:
            board.load();
            break;
          case SDLK_PERIOD:
            board.run();
            break;
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
          lastx = event.button.x;
          lasty = event.button.y;
          switch (event.button.button) {
          case SDL_BUTTON_LEFT:
            board.letlive_scaled(
                DoubleXY(lastx, lasty, windowsizex, windowsizey), state.loc);
            lbdown = 1;
            break;
          case SDL_BUTTON_RIGHT:
            board.letdie_scaled(
                DoubleXY(lastx, lasty, windowsizex, windowsizey), state.loc);
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
            auto xys =
                line(lastx, lasty, currx, curry, windowsizex, windowsizey);
            lastx = currx;
            lasty = curry;

            for (DoubleXY xy : xys) {
              if (lbdown) {
                board.letlive_scaled(std::move(xy), state.loc);
              } else if (rbdown) {
                board.letdie_scaled(std::move(xy), state.loc);
              }
            }
          }
          break;
        case SDL_MOUSEWHEEL: {
          int x, y;
          SDL_GetMouseState(&x, &y);
          DoubleXY f(x, y, windowsizex, windowsizey);

          if (event.wheel.y > 0) {
            state.zoomin(f.x, f.y);
          } else {
            state.zoomout(f.x, f.y);
          }
        } break;
        case SDL_WINDOWEVENT:
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            windowsizex = event.window.data1;
            windowsizey = event.window.data2;
            glViewport(0, 0, windowsizex, windowsizey);
          }
          break;
        }
      }
    }

    waiter.wait_if_fast();

    // draw allways
    state.draw(board.aliveactive.data(), SIZEX, SIZEY);
  }

  return 0;
}
