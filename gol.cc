#include <fstream>
#include <array>
#include <assert.h>
#include <list>
#include "../threadpool/threadpool.h"
#include <iostream>
#include "Waiter.h"

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

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

struct double_xy {
  double x, y;
  double_xy(int x_i, int y_i) : x(x_i), y(y_i) {
    x = (x / SIZEX) * 2.0 - 1.0;
    y = SIZEY - y; // down is positive in intcoord, negative in floatcoord
    y = (y / SIZEY) * 2.0 - 1.0;
    assert(x > -1.01 && x < 1.01);
    assert(y > -1.01 && y < 1.01);
  }
  double_xy(double x, double y) : x(x), y(y){};

  // 0.01 because rounding often made em SLIGHTLY below the real position
  int int_x() { return (int(SIZEX + SIZEX * x + 0.01)) / 2; }
  int int_y() { return (int(SIZEY - SIZEY * y + 0.01)) / 2; }
};

struct WindowScale {
  static const int SCALESIZE = 11;
  double t{1.0};
  double b{-1.0};
  double l{-1.0};
  double r{1.0};
  const double scalefactors[SCALESIZE] = {1.0, 1.4,  2.0,  2.8,  4.0, 5.6,
                                          8.0, 11.3, 16.0, 22.6, 32.0};
  unsigned scalefactor_i{0};

  void center_xy_on_xy(double_xy const &before, double_xy const &after) {
    double diff_x = before.x - after.x;
    double diff_y = before.y - after.y;

    t += diff_y;
    b += diff_y;
    l += diff_x;
    r += diff_x;

    stay_away_from_edges();
  }

  void scaled_to_view(double_xy *f) const {
    double dxy = (r - l) / 2.0;
    assert(dxy > 0.0 && dxy < 1.01); // positive because scale

    // Scale to correct sizing, move to middle of current view
    f->x = (f->x * dxy) + (l + r) / 2.0;
    f->y = (f->y * dxy) + (t + b) / 2.0;

    assert(f->x < 1.01 && f->x > -1.01 && f->y < 1.01 && f->y > -1.01);
  }

  void rescale_centering() {
    double dxy = 2.0 / scalefactors[scalefactor_i];
    assert(scalefactor_i > 5 || dxy > 0.1);

    double mid_y = (b + t) / 2.0;
    b = mid_y - dxy / 2.0;
    t = mid_y + dxy / 2.0;

    double mid_x = (l + r) / 2.0;
    l = mid_x - dxy / 2.0;
    r = mid_x + dxy / 2.0;

    stay_away_from_edges();
  }

  void stay_away_from_edges() {
    if (b < -1.0 && t > 1.0) {
      b = -1.0;
      t = 1.0;
    } else {
      if (b < -1.0) {
        t -= b + 1.0;
        b = -1.0;
      }
      if (t > 1.0) {
        b -= t - 1.0;
        t = 1.0;
      }
    }

    if (l < -1.0 && r > 1.0) {
      l = -1.0;
      r = 1.0;
    } else {
      if (l < -1.0) {
        r -= l + 1.0;
        l = -1.0;
      }
      if (r > 1.0) {
        l -= r - 1.0;
        r = 1.0;
      }
    }

    assert(b < t);
    assert(b > -1.01 && b < 1.01);
    assert(t > -1.01 && t < 1.01);
    assert(l < r);
    assert(l > -1.01 && l < 1.01);
    assert(r > -1.01 && r < 1.01);
  }

  void operator++() {
    if (scalefactor_i < (SCALESIZE - 1)) {
      ++scalefactor_i;
      rescale_centering();
    }
  }

  void operator--() {
    if (scalefactor_i > 0) {
      --scalefactor_i;
      rescale_centering();
    }
  }
};

struct Board {
  std::vector<unsigned char> aliveactive =
      std::vector<unsigned char>(SIZEX * SIZEY, 0);
  std::vector<unsigned char> alivewait =
      std::vector<unsigned char>(SIZEX * SIZEY, 0);
  std::vector<unsigned char> savedstate =
      std::vector<unsigned char>(SIZEX * SIZEY, 0);

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
    return (x >= 0 && y >= 0 && x < SIZEX && y < SIZEY);
  }

  bool aliveat(int x, int y) { return aliveactive[y * SIZEX + x] == 255; }

  unsigned char action(int x, int y) {
    if (x <= 0 || y <= 0 || x >= SIZEX - 1 || y >= SIZEY - 1) {
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
        return aliveactive[y * SIZEX + x] * 0.97;
      }
    }
  }

  void run() {
    int each = SIZEY / NRTHREADS;
    for (int i = 0; i < NRTHREADS; ++i) {
      pool([=]() {
        for (int y = i * each;
             y < (i + 1) * each || ((i + 1) == NRTHREADS && y < SIZEY); y++) {
          for (int x = 0; x < SIZEX; x++) {
            alivewait[y * SIZEX + x] = action(x, y);
          }
        }
      });
    }
    pool.wait_until_done();

    swap(aliveactive, alivewait);
  }

  void letlive_scaled(double_xy &&f, WindowScale const &loc) {
    loc.scaled_to_view(&f);
    letlive(f.int_x(), f.int_y());
  }

  void letdie_scaled(double_xy &&f, WindowScale const &loc) {
    loc.scaled_to_view(&f);
    letdie(f.int_x(), f.int_y());
  }

  void letlive(int x, int y) {
    if (safe_access(x, y)) {
      aliveactive[y * SIZEX + x] = 255;
    }
  }

  void letdie(int x, int y) {
    if (safe_access(x, y)) {
      aliveactive[y * SIZEX + x] = 0;
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

Board global_b;

const char *vertexshader = "#version 150\n"
                           "in vec2 position;\n"
                           "in vec2 texcoord;\n"
                           "out vec2 Texcoord;\n"
                           "void main(){Texcoord = texcoord; gl_Position = "
                           "vec4(position, 0.0, 1.0);}\n";

const char *fragshader =
    "#version 150\n"
    "in vec2 Texcoord;\n"
    "out vec4 outColor;\n"
    "uniform sampler2D tex;\n"
    "void main(){vec4 inp = texture(tex, Texcoord); if (inp.r > 0.95 || inp.r "
    "< 0.01) {outColor = vec4(inp.r, inp.r, inp.r, inp.r);} else {outColor = "
    "vec4(sin(inp.r*3.14), 0.0, sin(inp.r*32)*0.4, inp.r);}}\n";

struct GLstate {
  GLuint vao, vbo, ebo, tex, shaderProgram, fragmentShader, vertexShader;
  SDL_GLContext context;
  SDL_Window *window;
  WindowScale loc;
  // posx, posy, texx, texy, clockwise starting top-left
  float vertices[16]{-1.0, 1.0,  0.0, 0.0, 1.0,  1.0,  0.0, 0.0,
                     1.0,  -1.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0};

  void zoomin(double x, double y) {
    double_xy before(x, y);
    double_xy after(x, y);

    loc.scaled_to_view(&before);
    ++loc;
    loc.scaled_to_view(&after);

    loc.center_xy_on_xy(before, after);

    setVertMatrix();
  }

  void zoomout(double x, double y) {
    double_xy before(x, y);
    double_xy after(x, y);

    loc.scaled_to_view(&before);
    --loc;
    loc.scaled_to_view(&after);

    loc.center_xy_on_xy(before, after);

    setVertMatrix();
  }

  void setVertMatrix() {
    vertices[2] = (loc.l + 1.0) / 2.0;
    vertices[3] = (-loc.t + 1.0) / 2.0;
    vertices[6] = (loc.r + 1.0) / 2.0;
    vertices[7] = (-loc.t + 1.0) / 2.0;
    vertices[10] = (loc.r + 1.0) / 2.0;
    vertices[11] = (-loc.b + 1.0) / 2.0;
    vertices[14] = (loc.l + 1.0) / 2.0;
    vertices[15] = (-loc.b + 1.0) / 2.0;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZEX, SIZEY, 0, GL_RED,
                 GL_UNSIGNED_BYTE, global_b.aliveactive.data());

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    SDL_GL_SwapWindow(window);
  }

  void sdlglewinit() {
    // SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    window = SDL_CreateWindow("fizzos Game of Life", 100, 100, SIZEX, SIZEY,
                              SDL_WINDOW_OPENGL);
    context = SDL_GL_CreateContext(window);

    // GLEW
    glewExperimental = GL_TRUE;
    glewInit();
  }

  void vabetexinit() {
    // VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // vbo
    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    setVertMatrix();

    // element buffer
    GLuint ebo;
    glGenBuffers(1, &ebo);
    GLuint elements[] = {0, 1, 2, 2, 3, 0};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                 GL_STATIC_DRAW);

    // tex
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  void shadersinit() {
    // vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexshader, NULL);
    glCompileShader(vertexShader);

    // fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragshader, NULL);
    glCompileShader(fragmentShader);

    // shader prog
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

#ifndef NDEBUG
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE && "vshader felkompilerad");
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE && "fshader felkompilerad");

    char compilelog[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, compilelog);

    GLenum errvar = glGetError();
    // assert(errvar != GL_INVALID_ENUM); //Can be ignored, no reason to exit
    assert(errvar != GL_INVALID_VALUE);
    assert(errvar != GL_INVALID_OPERATION);
    assert(errvar != GL_INVALID_FRAMEBUFFER_OPERATION);
#endif

    // data layout
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          0);

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
  }

  GLstate() {
    sdlglewinit();
    vabetexinit();
    shadersinit();
  }

  ~GLstate() {
    glDeleteTextures(1, &tex);
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(context);
    SDL_Quit();
  }
};

std::vector<double_xy> line(int x0, int y0, int x1, int y1) {
  std::vector<double_xy> ret;

  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int err2;

  while (true) {
    ret.emplace_back(x0, y0);
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

  std::ofstream logfile;
  logfile.open("log");
  if (!logfile) {
    assert(0 && "logfile error");
    return 0;
  }

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
            for (int y = 0; y < SIZEY; y++) {
              for (int x = 0; x < SIZEX; x++) {
                logfile << (global_b.aliveactive[y * SIZEX + x] == 255 ? 'O'
                                                                       : ' ');
              }
              logfile << std::endl;
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
            global_b.letlive_scaled(double_xy(lastx, lasty), state.loc);
            lbdown = 1;
            break;
          case SDL_BUTTON_RIGHT:
            global_b.letdie_scaled(double_xy(lastx, lasty), state.loc);
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
            auto xys = line(lastx, lasty, currx, curry);
            lastx = currx;
            lasty = curry;

            for (double_xy xy : xys) {
              if (lbdown) {
                global_b.letlive_scaled(std::move(xy), state.loc);
              } else if (rbdown) {
                global_b.letdie_scaled(std::move(xy), state.loc);
              }
            }
          }
          break;
        case SDL_MOUSEWHEEL:
          int x, y;
          SDL_GetMouseState(&x, &y);
          double_xy f(x, y);

          if (event.wheel.y > 0) {
            state.zoomin(f.x, f.y);
          } else {
            state.zoomout(f.x, f.y);
          }
          break;
        }
      }
    }

    waiter.wait_if_fast();

    // draw allways
    state.draw();
  }

  return 0;
}
