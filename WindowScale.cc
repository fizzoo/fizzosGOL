#include "WindowScale.h"

void WindowScale::center_xy_on_xy(DoubleXY const &before,
                                  DoubleXY const &after) {
  double diff_x = before.x - after.x;
  double diff_y = before.y - after.y;

  t += diff_y;
  b += diff_y;
  l += diff_x;
  r += diff_x;

  stay_away_from_edges();
}

void WindowScale::scaled_to_view(DoubleXY *f) const {
  double dxy = (r - l) / 2.0;
  assert(dxy > 0.0 && dxy < 1.01); // positive because scale

  // Scale to correct sizing, move to middle of current view
  f->x = (f->x * dxy) + (l + r) / 2.0;
  f->y = (f->y * dxy) + (t + b) / 2.0;

  assert(f->x < 1.01 && f->x > -1.01 && f->y < 1.01 && f->y > -1.01);
}

void WindowScale::rescale_centering() {
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

void WindowScale::stay_away_from_edges() {
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

void WindowScale::operator++() {
  if (scalefactor_i < (SCALESIZE - 1)) {
    ++scalefactor_i;
    rescale_centering();
  }
}

void WindowScale::operator--() {
  if (scalefactor_i > 0) {
    --scalefactor_i;
    rescale_centering();
  }
}
