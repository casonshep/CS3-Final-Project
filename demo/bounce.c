#include "color.h"
#include "list.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double TWO_PI = 2.0 * M_PI;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// Star constants
const size_t FIRST_STAR_NUM_OUTER_POINTS = 10;
const double INNER_STAR_RADIUS = 50;
const double OUTER_STAR_RADIUS = 100;

// Evolution constants
const vector_t INITIAL_VELOCITY = {.x = 50, .y = 50};
const double ANGULAR_VELOCITY = TWO_PI / 1000;

// Color constants
const rgb_color_t STAR_COLOR = COLOR_YELLOW;

/**
 * Returns a star-shaped polygon
 * Relative to the center, vertices are spaced at equal angles and alternate
 * between the radii, starting with the outer radius.
 */
list_t *make_star(double inner_radius, double outer_radius, size_t num_points,
                  vector_t center) {
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

/**
 * Detects which wall the given star polygon is inside of, if any
 * Returns 'l' if in the left wall, 'r' if in the right wall, 'b' if in the
 * bottom wall, 't' if in the top wall, and the null character if not in any
 * wall.
 */
char hitting_wall(list_t *vertices) {
  size_t num_vertices = list_size(vertices);

  // Iterate only through outer vertices (even indices)
  for (size_t i = 0; i < num_vertices; i += 2) {
    vector_t *vertex = list_get(vertices, i);
    if (vertex->x <= 0) {
      return 'l';
    }
    if (vertex->x >= WINDOW.x) {
      return 'r';
    }
    if (vertex->y <= 0) {
      return 'b';
    }
    if (vertex->y >= WINDOW.y) {
      return 't';
    }
  }
  return '\0';
}

struct state {
  list_t *star;
  vector_t *velocity;
};

state_t *emscripten_init() {
  // Initialize window and renderer
  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  state->star = make_star(INNER_STAR_RADIUS, OUTER_STAR_RADIUS,
                          FIRST_STAR_NUM_OUTER_POINTS, CENTER);
  state->velocity = malloc(sizeof(vector_t));
  *(state->velocity) = INITIAL_VELOCITY;

  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  list_t *star = state->star;

  // Change sign of velocity to escape from whichever wall it is inside of
  char wall_hit = hitting_wall(star);
  switch (wall_hit) {
  case 'l':
    state->velocity->x = fabs(state->velocity->x);
    break;
  case 'r':
    state->velocity->x = -fabs(state->velocity->x);
    break;
  case 'b':
    state->velocity->y = fabs(state->velocity->y);
    break;
  case 't':
    state->velocity->y = -fabs(state->velocity->y);
    break;
  }

  // Move and rotate the star
  vector_t distance = vec_multiply(dt, *(state->velocity));
  polygon_translate(star, distance);
  polygon_rotate(star, ANGULAR_VELOCITY, polygon_centroid(star));

  // Display the star
  sdl_draw_polygon(star, STAR_COLOR);
  sdl_show();
}

void emscripten_free(state_t *state) {
  list_t *star = state->star;
  list_free(star);
  free(state);
}
