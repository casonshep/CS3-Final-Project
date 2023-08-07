#include "color.h"
#include "test_util.h"

#include <assert.h>
#include <math.h>

void test_hue() {
  assert(isclose(color_hue(COLOR_AQUA), deg_to_rad(180)));
  assert(isclose(color_hue((rgb_color_t){.6, .4, .2}), deg_to_rad(30)));
  assert(isclose(color_hue((rgb_color_t){.5, .5, .1}), deg_to_rad(60)));
  assert(isclose(color_hue((rgb_color_t){.3, .9, .45}), deg_to_rad(135)));
}

void test_sat() {
  assert(isclose(color_sat(COLOR_AQUA), 1));
  assert(isclose(color_sat((rgb_color_t){.6, .4, .2}), .6666666));
  assert(isclose(color_sat((rgb_color_t){.5, .5, .1}), .8));
}

void test_color_from_hsv() {
  assert(color_equal(color_from_hsv(0, 1, 1), COLOR_RED));
  assert(color_equal(color_from_hsv(0, 0, 0), COLOR_BLACK));
  assert(color_equal(color_from_hsv(0, 1, .8), (rgb_color_t){.8, 0, 0}));
  assert(color_equal(color_from_hsv(M_PI, 1, 1), COLOR_AQUA));
  assert(color_equal(color_from_hsv(M_PI, .5, .5), (rgb_color_t){.25, .5, .5}));
  assert(color_equal(color_from_hsv(0, .5, 1), (rgb_color_t){1, .5, .5}));
}

void test_color_hue_shift() {
  assert(color_equal(color_hue_shift(COLOR_RED, M_PI), COLOR_AQUA));
  assert(color_equal(color_hue_shift(COLOR_LIME, M_PI), COLOR_FUCHSIA));
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_hue)
  DO_TEST(test_sat)
  DO_TEST(test_color_from_hsv)
  DO_TEST(test_color_hue_shift)

  puts("rgb_color_test PASS");
}