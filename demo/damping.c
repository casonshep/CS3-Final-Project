#include "color.h"
#include "forces.h"
#include "list.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

// Mathematical Constants
const double TWO_PI = 2.0 * M_PI;

// Circle constants
const size_t NUM_COLORED_CIRCLES = 50;
const size_t NUM_TOTAL_CIRCLES = 100;
const size_t CIRCLE_POINTS = 40;

// Force constants
const double INIT_K = 0.2 * NUM_COLORED_CIRCLES;
const double K_SHIFT = -0.2; // Change

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// Mass constants
const double COLORED_CIRCLE_MASS = 10; // May need to change
const double INVISIBLE_CIRCLE_MASS = INFINITY;

// Color constants
const rgb_color_t FIRST_CIRCLE_COLOR = COLOR_AQUA;
const float HUE_SHIFT = (float)(TWO_PI / 50);

/**
 * Generates circle vertices list given radius of circle, number of vertices
 * of circcle, and center coordinates (vector_t) of circle.
 * Returns list_t of vertices of circle.
 */
list_t *make_circle(size_t radius, size_t num_points, vector_t center) {
  // Initialize angles and circle
  double curr_angle = 0;
  double vert_angle = TWO_PI / num_points;
  list_t *vertices = list_init(num_points, free);
  assert(vertices != NULL);

  // Add vertices to circle
  for (size_t i = 0; i < num_points; i++) {
    // Calculate cartesian coordinates
    double x = cos(curr_angle) * radius + center.x;
    double y = sin(curr_angle) * radius + center.y;

    // Create vertex
    vector_t *vec_ptr = malloc(sizeof(vector_t));
    vec_ptr->x = x;
    vec_ptr->y = y;

    list_add(vertices, vec_ptr);
    curr_angle += vert_angle;
  }

  return vertices;
}

struct state {
  scene_t *scene;
};

// Init
// spawn 50 circles with varying colors across screen
// 1 white circle for each location, too
// white circles of infinite mass, colored circles of finite mass
// apply spring force between each, gamma damping const increases as moving
// across
state_t *emscripten_init() {

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;

  // Initialize window and renderer
  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  // Create colored-invisible circle pairs and add to the scene
  double circle_radius = WINDOW.x / (2 * NUM_COLORED_CIRCLES);
  vector_t first_circle_center = {.x = circle_radius,
                                  .y = WINDOW.y - circle_radius};
  vector_t center_shift = {.x = 2 * circle_radius, .y = 0};
  rgb_color_t curr_color = FIRST_CIRCLE_COLOR;
  vector_t curr_center = first_circle_center;
  for (size_t i = 0; i < NUM_COLORED_CIRCLES; ++i) {
    vector_t invis_circle_center = {.x = curr_center.x, .y = CENTER.y};
    list_t *invis_circle_vertices =
        make_circle(circle_radius, CIRCLE_POINTS, invis_circle_center);
    body_t *invis_circle =
        body_init(invis_circle_vertices, INVISIBLE_CIRCLE_MASS, COLOR_WHITE);
    scene_add_body(state->scene, invis_circle);

    list_t *colored_circle_vertices =
        make_circle(circle_radius, CIRCLE_POINTS, curr_center);
    body_t *colored_circle =
        body_init(colored_circle_vertices, COLORED_CIRCLE_MASS, curr_color);
    scene_add_body(state->scene, colored_circle);

    curr_color = color_hue_shift(curr_color, HUE_SHIFT);
    curr_center = vec_add(curr_center, center_shift);
  }

  // Create forces between the circle pairs
  double curr_k = INIT_K;
  for (size_t i = 0; i < NUM_TOTAL_CIRCLES; i += 2) {
    body_t *invis_circle = scene_get_body(state->scene, i);
    body_t *colored_circle = scene_get_body(state->scene, i + 1);
    create_spring(state->scene, curr_k, colored_circle, invis_circle);

    curr_k += K_SHIFT;
  }

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  scene_tick(state->scene, dt);
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}