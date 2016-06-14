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

/**
 * Class relating to everything required to put some graphics on the screen.
 */
class GLstate {
private:
  GLuint vao, vbo, ebo, tex, shaderProgram, fragmentShader, vertexShader;
  SDL_GLContext context;
  SDL_Window *window;
  // posx, posy, texx, texy, clockwise starting top-left
  float vertices[16]{-1.0, 1.0,  0.0, 0.0, 1.0,  1.0,  0.0, 0.0,
                     1.0,  -1.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0};

  void sdlglewinit();
  void vabetexinit();
  void shadersinit();
  void setVertMatrix();

public:
  WindowScale loc;

  /**
   * Constructors initialize a bunch of SDL / GL things, use only one per
   * program!
   */
  GLstate();
  ~GLstate();

  /**
   * Zooms with respect to the window-pixels.
   */
  void zoomin(double x, double y);
  void zoomout(double x, double y);

  /**
   * Draws the data on the screen.
   */
  void draw(unsigned char *data, int sizex, int sizey);
};

#endif /* end of include guard: GLSTATE_H */
