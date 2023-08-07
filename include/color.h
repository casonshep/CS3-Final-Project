#ifndef __COLOR_H__
#define __COLOR_H__

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components.
 * Each component must be between 0 (black) and 1 (white).
 */
typedef struct {
  float r;
  float g;
  float b;
} rgb_color_t;

#define COLOR_BLACK ((rgb_color_t){0, 0, 0})
#define COLOR_WHITE ((rgb_color_t){1, 1, 1})
#define COLOR_RED ((rgb_color_t){1, 0, 0})
#define COLOR_YELLOW ((rgb_color_t){1, 1, 0})
#define COLOR_GREEN ((rgb_color_t){0, 0.8, 0})
#define COLOR_LIME ((rgb_color_t){0, 1, 0})
#define COLOR_AQUA ((rgb_color_t){0, 1, 1})
#define COLOR_BLUE ((rgb_color_t){0, 0, 1})
#define COLOR_FUCHSIA ((rgb_color_t){1, 0, 1})
#define COLOR_VIOLET ((rgb_color_t){0.5, 0, 0.5})
#define COLOR_ORANGE ((rgb_color_t){1, 0.64, 0})
#define COLOR_INDIGO ((rgb_color_t){0.294, 0, 0.51})

/**
 * Adds the RGB components of each color. If any component in the sum exceeds 1,
 * that component will be set to 1.
 */
rgb_color_t color_add(rgb_color_t c1, rgb_color_t c2);

/**
 * Subtracts the RGB components of c2 from those of c1. If any component in the
 * difference is less than 0, that component will be set to 0.
 * Calclated using formula from:
 * https://www.rapidtables.com/convert/color/rgb-to-hsv.html
 */
rgb_color_t color_subtract(rgb_color_t c1, rgb_color_t c2);

/**
 * Returns the HSV hue of a color in radians, ranging from 0 (inclusive) to 2*pi
 * (exclusive), with 0 corresponding to red. Returns 0 for black.
 */
float color_hue(rgb_color_t c);

/**
 * Returns the HSV saturation of a color, ranging from 0 to 1 (inclusive).
 */
float color_sat(rgb_color_t c);

/**
 * Returns the HSV value of a color, ranging from 0 to 1 (inclusive).
 */
float color_val(rgb_color_t c);

/**
 * Returns a rgb_color_t with the given HSV components, with hue in radians.
 * Calculated using formula from:
 * https://www.rapidtables.com/convert/color/hsv-to-rgb.html
 */
rgb_color_t color_from_hsv(float h, float s, float v);

/**
 * Returns a rgb_color_t with the same saturation and value as the given color,
 * but with a hue equal to the sum of the original hue and hue_change (given in
 * radians).
 */
rgb_color_t color_hue_shift(rgb_color_t c, float hue_change);

#endif // #ifndef __COLOR_H__
