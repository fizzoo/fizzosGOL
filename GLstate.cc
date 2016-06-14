#include "GLstate.h"

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

void GLstate::zoomin(double x, double y) {
  DoubleXY before(x, y);
  DoubleXY after(x, y);

  loc.scaled_to_view(&before);
  ++loc;
  loc.scaled_to_view(&after);

  loc.center_xy_on_xy(before, after);

  setVertMatrix();
}

void GLstate::zoomout(double x, double y) {
  DoubleXY before(x, y);
  DoubleXY after(x, y);

  loc.scaled_to_view(&before);
  --loc;
  loc.scaled_to_view(&after);

  loc.center_xy_on_xy(before, after);

  setVertMatrix();
}

void GLstate::setVertMatrix() {
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

void GLstate::draw(unsigned char *data, int sizex, int sizey) {
  glClear(GL_COLOR_BUFFER_BIT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sizex, sizey, 0, GL_RED,
      GL_UNSIGNED_BYTE, data);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  SDL_GL_SwapWindow(window);
}

void GLstate::sdlglewinit() {
  // SDL
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
      SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  window = SDL_CreateWindow("fizzos Game of Life", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  context = SDL_GL_CreateContext(window);

  // GLEW
  glewExperimental = GL_TRUE;
  glewInit();
}

void GLstate::vabetexinit() {
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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void GLstate::shadersinit() {
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

GLstate::GLstate() {
  sdlglewinit();
  vabetexinit();
  shadersinit();
}

GLstate::~GLstate() {
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
