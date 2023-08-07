#include "color.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

rgb_color_t color_add(rgb_color_t c1, rgb_color_t c2) {
  c1.r = fminf(c1.r + c2.r, 1);
  c1.b = fminf(c1.b + c2.b, 1);
  c1.g = fminf(c1.g + c2.g, 1);
  return c1;
}

rgb_color_t color_subtract(rgb_color_t c1, rgb_color_t c2) {
  c1.r = fmaxf(c1.r - c2.r, 0);
  c1.b = fmaxf(c1.b - c2.b, 0);
  c1.g = fmaxf(c1.g - c2.g, 0);
  return c1;
}

float color_hue(rgb_color_t c) {
  float max = color_val(c);
  float hue = 0.0;

  float min = fminf(c.r, c.g);
  min = fminf(min, c.b);
  float delta = max - min;
  if (delta == 0) {
    return 0.0;
  }

  // calculate hue (if delta != 0)
  if (max == c.r) {
    hue = M_PI / 3 * (c.g - c.b) / delta;
  } else if (max == c.g) {
    hue = M_PI / 3 * (((c.b - c.r) / delta) + 2);
  } else if (max == c.b) {
    hue = M_PI / 3 * (((c.r - c.g) / delta) + 4);
  }

  if (hue < 0) {
    hue += 2.0 * M_PI;
  }

  return hue;
}

float color_sat(rgb_color_t c) {
  float max = color_val(c);
  if (max == 0) {
    return 0;
  }

  float min = fminf(c.r, c.g);
  min = fminf(min, c.b);
  float delta = max - min;

  return delta / max;
}

float color_val(rgb_color_t c) {
  float max = 0;
  if (c.r > c.g && c.r > c.b) {
    max = c.r;
  } else if (c.b > c.g && c.b > c.r) {
    max = c.b;
  } else {
    max = c.g;
  }
  return max;
}

rgb_color_t color_from_hsv(float h, float s, float v) {
  float hue_degrees = (h * (180 / M_PI));
  float r_prime = 0.0;
  float g_prime = 0.0;
  float b_prime = 0.0;
  float c = v * s;
  float H = hue_degrees / 60;

  // manual modulo to preserve decimal value
  while (H > 2) {
    H -= 2;
  }

  float x = c * (1 - fabs(H - 1));
  float m = v - c;

  if (hue_degrees >= 0 && hue_degrees < 60) {
    r_prime = c;
    g_prime = x;
    b_prime = 0;
  } else if (hue_degrees < 120) {
    r_prime = x;
    g_prime = c;
    b_prime = 0;
  } else if (hue_degrees < 180) {
    r_prime = 0;
    g_prime = c;
    b_prime = x;
  } else if (hue_degrees < 240) {
    r_prime = 0;
    g_prime = x;
    b_prime = c;
  } else if (hue_degrees < 300) {
    r_prime = x;
    g_prime = 0;
    b_prime = c;
  } else if (hue_degrees < 360) {
    r_prime = c;
    g_prime = 0;
    b_prime = x;
  }
  rgb_color_t color = {(r_prime + m), (g_prime + m), (b_prime + m)};

  return color;
}

rgb_color_t color_hue_shift(rgb_color_t c, float hue_change) {
  float s = color_sat(c);
  float v = color_val(c);
  float h = color_hue(c) + hue_change;
  if (h < 0) {
    while (h < 0) {
      h += (2 * M_PI);
    }
  } else if (h >= 2 * M_PI) {
    while (h >= 2 * M_PI) {
      h -= 2 * M_PI;
    }
  }
  return color_from_hsv(h, s, v);
}