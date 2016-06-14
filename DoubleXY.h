#ifndef DOUBLEXY_H
#define DOUBLEXY_H

#include <cassert>

/**
 * Struct helping with dealing with gl-double-space (-1, 1) and transfers to
 * and from the regular grid-int-space.
 */
struct DoubleXY {
  double x, y;

  DoubleXY(double x, double y) : x(x), y(y){};
  DoubleXY(int x, int y) = delete;

  DoubleXY(int x_i, int y_i, int maxx, int maxy) : x(x_i), y(y_i){
    x = (x / maxx) * 2.0 - 1.0;
    y = maxy - y; // down is positive in intcoord, negative in floatcoord
    y = (y / maxy) * 2.0 - 1.0;
    assert(x > -1.01 && x < 1.01);
    assert(y > -1.01 && y < 1.01);
  }

  // 0.01 because rounding often made em SLIGHTLY below the real position
  int int_x(int maxx) { return (int(maxx + maxx * x + 0.01)) / 2; }
  int int_y(int maxy) { return (int(maxy - maxy * y + 0.01)) / 2; }
};

#endif /* end of include guard: DOUBLEXY_H */
