#ifndef GLSTATE_H
#define GLSTATE_H

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "WindowScale.h"

struct GLstate {
  GLuint vao, vbo, ebo, tex, shaderProgram, fragmentShader, vertexShader;
  SDL_GLContext context;
  SDL_Window *window;
  WindowScale loc;
  // posx, posy, texx, texy, clockwise starting top-left
  float vertices[16]{-1.0, 1.0,  0.0, 0.0, 1.0,  1.0,  0.0, 0.0,
                     1.0,  -1.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0};

  void zoomin(double x, double y);
  void zoomout(double x, double y);
  void setVertMatrix();
  void draw(unsigned char *data, int sizex, int sizey);
  void sdlglewinit();
  void vabetexinit();
  void shadersinit();
  GLstate();
  ~GLstate();
};

#endif /* end of include guard: GLSTATE_H */
