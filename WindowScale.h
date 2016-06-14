#ifndef WINDOWSCALE_H
#define WINDOWSCALE_H

#include "DoubleXY.h"

struct WindowScale {
  static const int SCALESIZE = 11;
  double t{1.0};
  double b{-1.0};
  double l{-1.0};
  double r{1.0};
  const double scalefactors[SCALESIZE] = {1.0, 1.4,  2.0,  2.8,  4.0, 5.6,
                                          8.0, 11.3, 16.0, 22.6, 32.0};
  unsigned scalefactor_i{0};

  void center_xy_on_xy(DoubleXY const &before, DoubleXY const &after);
  void scaled_to_view(DoubleXY *f) const;
  void rescale_centering();
  void stay_away_from_edges();
  void operator++();
  void operator--();
};

#endif /* end of include guard: WINDOWSCALE_H */
