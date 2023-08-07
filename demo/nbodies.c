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
#include <time.h>

// Mathematical Constants
const double TWO_PI = 2.0 * M_PI;
const double G = 1000;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// Star constants
const size_t NUM_STARS = 100;
const size_t STAR_NUM_OUTER_POINTS = 4;
const double STAR_MIN_OUTER_RADIUS = 10;
const double STAR_MAX_OUTER_RADIUS = 30;
const double OUTER_RADIUS_MASS_RATIO = 0.5;

// Color constants
const rgb_color_t FIRST_STAR_COLOR = COLOR_RED;
const float HUE_SHIFT = (float)(TWO_PI / 12);

/**
 * Generates a random outer_radius for a star.
 */
double random_radius() {
  return STAR_MIN_OUTER_RADIUS +
         ((double)rand() / RAND_MAX *
          (STAR_MAX_OUTER_RADIUS - STAR_MIN_OUTER_RADIUS));
}

/**
 * Generates a random center coordinate to generate a star at.
 */
vector_t random_loc() {
  return (vector_t){.x = (double)rand() / RAND_MAX * WINDOW.x,
                    .y = (double)rand() / RAND_MAX * WINDOW.y};
}

/**
 * Returns a star-shaped polygon
 * Relative to the center, vertices are spaced at equal angles and alternate
 * between the radii, starting with the outer radius.
 */
list_t *make_star(double outer_radius, size_t num_outer_points,
                  vector_t center) {
  size_t num_points = 2 * num_outer_points;
  double inner_radius = outer_radius / 2;

  double vertex_angle = TWO_PI / num_points;
  list_t *vertices = list_init(num_points, free);
  assert(vertices != NULL);

  for (size_t i = 0; i < num_points; ++i) {
    // Calculate polar coordinates relative to center
    double radius = (i % 2 == 0) ? outer_radius : inner_radius;
    double angle = i * vertex_angle;

    // Calculate cartesian coordinates relative to (0, 0)
    double x = cos(angle) * radius + center.x;
    double y = sin(angle) * radius + center.y;

    // Create and add vertex
    vector_t *vertex = malloc(sizeof(vector_t));
    vertex->x = x;
    vertex->y = y;
    list_add(vertices, vertex);
  }

  return vertices;
}

struct state {
  scene_t *scene;
};

state_t *emscripten_init() {
  srand(time(NULL));

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;

  // Initialize window and renderer
  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  // Create stars and add to the scene
  rgb_color_t curr_color = FIRST_STAR_COLOR;
  for (size_t i = 0; i < NUM_STARS; ++i) {
    double radius = random_radius();
    vector_t center = random_loc();
    double mass = radius * OUTER_RADIUS_MASS_RATIO;

    list_t *vertices = make_star(radius, STAR_NUM_OUTER_POINTS, center);
    body_t *star = body_init(vertices, mass, curr_color);
    scene_add_body(state->scene, star);
    curr_color = color_hue_shift(curr_color, HUE_SHIFT);
  }

  // Create forces between stars
  for (size_t i = 0; i < NUM_STARS; ++i) {
    for (size_t j = i + 1; j < NUM_STARS; ++j) {
      body_t *body1 = scene_get_body(state->scene, i);
      body_t *body2 = scene_get_body(state->scene, j);
      create_newtonian_gravity(state->scene, G, body1, body2);
    }
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