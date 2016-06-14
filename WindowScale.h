#ifndef WINDOWSCALE_H
#define WINDOWSCALE_H

#include "DoubleXY.h"

class WindowScale {
private:
  static const int SCALESIZE = 11;
  const double scalefactors[SCALESIZE] = {1.0, 1.4,  2.0,  2.8,  4.0, 5.6,
                                          8.0, 11.3, 16.0, 22.6, 32.0};
  unsigned scalefactor_i{0};

  void stay_away_from_edges();
  void rescale_centering();

public:
  //Top, bottom, left, right edges
  double t{1.0};
  double b{-1.0};
  double l{-1.0};
  double r{1.0};

  /**
   * Moves the window such that the 'before', as taken from before a zoom or
   * move, is on the same spot on the screen as the 'after'.
   */
  void center_xy_on_xy(DoubleXY const &before, DoubleXY const &after);

  /**
   * Modifies the doublexy with respects to the window scale, in order to map
   * it from the (-1, 1) of the window to a smaller set of that, depending on
   * where our window is at.
   */
  void scaled_to_view(DoubleXY *f) const;

  /**
   * Zooms the scale in/out.
   */
  void operator++();
  void operator--();
};

#endif /* end of include guard: WINDOWSCALE_H */
