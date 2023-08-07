#include "state.h"

#include "body.h"
#include "color.h"
#include "polygon.h"
#include "sdl_wrapper.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // This is for seeding the RNG; simulation time is handled by Emscripten

// Simulation parameters
const size_t NUM_STARS = 7;
const double MIN_ELASTICITY = 0.8;
const double MAX_ELASTICITY = 0.9;
const double GROUND_HEIGHT = 40;

// Mathematical constants
const double TWO_PI = 2.0 * M_PI;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};

// Star constants
const size_t FIRST_STAR_NUM_OUTER_POINTS = 2;
const size_t MAX_NUM_OUTER_POINTS = 20;
const size_t NUM_OUTER_POINTS_INCREMENT = 1;
const double INNER_STAR_RADIUS = 20;
const double OUTER_STAR_RADIUS = 40;
const double MASS = 0;

// Color constants
const rgb_color_t GROUND_COLOR = {0.5, 0.5, 0.5};
const rgb_color_t FIRST_STAR_COLOR = COLOR_RED;
const float HUE_SHIFT = (float)(TWO_PI / 12);

// Evolution constants
const double SPAWN_DELAY_FACTOR = 0.01;
const vector_t INITIAL_POSITION = {0, 500};
const vector_t INITIAL_VELOCITY = {0.5, 0};
const double ANGULAR_VELOCITY = TWO_PI / 1000;
const vector_t GRAV_ACC = {0, -1.5};
// BOUNCE_CORRECTION is added to the y component of the velocity of stars that
// bounce due to rotation. Stars stuck in the ground for more than 1 frame are
// ensured to have a y-velocity of at least BOUNCE_CORRECTION.
const double BOUNCE_CORRECTION = 1;

/**
 * Returns a rectangular polygon for the ground
 */
list_t *make_ground() {
  vector_t *bl = malloc(sizeof(vector_t));
  vector_t *br = malloc(sizeof(vector_t));
  vector_t *tr = malloc(sizeof(vector_t));
  vector_t *tl = malloc(sizeof(vector_t));

  *bl = (vector_t){0, 0};
  *br = (vector_t){WINDOW.x, 0};
  *tr = (vector_t){WINDOW.x, GROUND_HEIGHT};
  *tl = (vector_t){0, GROUND_HEIGHT};

  list_t *ground = list_init(4, free);

  list_add(ground, bl);
  list_add(ground, br);
  list_add(ground, tr);
  list_add(ground, tl);

  return ground;
}

/**
 * Returns a star-shaped polygon
 * Relative to the center, vertices are spaced at equal angles and alternate
 * between the radii, starting with the outer radius.
 * If num_outer_points is 3, this function returns a triangle instead of a
 * 3-pointed star.
 */
list_t *make_star(double inner_radius, double outer_radius,
                  size_t num_outer_points, vector_t center) {
  size_t num_points = 2 * num_outer_points;

  // Correct for triangles
  if (num_outer_points == 3) {
    inner_radius = outer_radius;
    num_points = num_outer_points;
  }

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
 * Returns a random elasticity value, between MIN_ELASTICITY and MAX_ELASTICITY
 */
double get_elasticity() {
  return MIN_ELASTICITY +
         (double)rand() / RAND_MAX * (MAX_ELASTICITY - MIN_ELASTICITY);
}

typedef enum special_state {
  NO_SPECIAL_STATE,
  IN_GROUND,
  OFF_SCREEN
} special_state_t;

/**
 * Returns OFF_SCREEN if the star is off-screen, IN_GROUND if the star is
 * on-screen and in the ground, and NO_SPECIAL_STATE otherwise;
 */
special_state_t get_special_state(list_t *vertices) {
  size_t num_vertices = list_size(vertices);

  // If there are 2 or 3 vertices, the polygon is a diamond or triangle, so all
  // of its vertices are outer.
  size_t index_increment = (num_vertices <= 3) ? 1 : 2;

  bool off_screen = true;
  bool in_ground = false;

  // Iterate only through outer vertices
  for (size_t i = 0; i < num_vertices; i += index_increment) {
    vector_t *vertex = list_get(vertices, i);
    if (vertex->x <= WINDOW.x) {
      off_screen = false;
    }
    if (vertex->y <= GROUND_HEIGHT) {
      in_ground = true;
    }
  }
  return off_screen ? OFF_SCREEN : (in_ground ? IN_GROUND : NO_SPECIAL_STATE);
}

struct state {
  list_t *stars;
  size_t next_num_outer_points;
  rgb_color_t next_color;
  double time_until_next_spawn;
};

state_t *emscripten_init() {
  // Seed the RNG
  srand(time(NULL));

  // Initialize window and renderer
  vector_t min = {0, 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  state->stars = list_init(NUM_STARS + 1, (void *)body_free);
  state->next_num_outer_points = FIRST_STAR_NUM_OUTER_POINTS;
  state->next_color = FIRST_STAR_COLOR;
  state->time_until_next_spawn = 0;

  return state;
}

// This should be obtained from body.h (implemented in body.c), but for some
// reason, the linker can't find it, and compilation fails. The body_evolve
// function is copied here as a hack.
void body_evolve(body_t *body, double dt, vector_t avg_acc,
                 double avg_ang_acc) {
  vector_t velocity = body_get_velocity(body);
  vector_t avg_velocity = vec_add(velocity, vec_multiply(dt / 2, avg_acc));

  body_set_centroid(body, vec_add(body_get_centroid(body), avg_velocity));
  body_set_rotation(body, body_get_rotation(body) + ANGULAR_VELOCITY);

  vector_t updated_velocity = vec_add(velocity, vec_multiply(dt, avg_acc));
  body_set_velocity(body, updated_velocity);
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  state->time_until_next_spawn -= dt;

  // Spawn a new star if appropriate
  bool star_just_spawned = false;
  if (state->time_until_next_spawn <= 0) {
    star_just_spawned = true;

    list_t *shape = make_star(INNER_STAR_RADIUS, OUTER_STAR_RADIUS,
                              state->next_num_outer_points, INITIAL_POSITION);

    body_t *spawned_star = body_init(shape, MASS, state->next_color);
    body_set_velocity(spawned_star, INITIAL_VELOCITY);
    list_add(state->stars, spawned_star);

    state->next_num_outer_points += NUM_OUTER_POINTS_INCREMENT;
    if (state->next_num_outer_points > MAX_NUM_OUTER_POINTS) {
      state->next_num_outer_points = MAX_NUM_OUTER_POINTS;
    }
    rgb_color_t curr_color = state->next_color;
    state->next_color = color_hue_shift(curr_color, HUE_SHIFT);
    state->time_until_next_spawn =
        SPAWN_DELAY_FACTOR * WINDOW.x / (NUM_STARS * INITIAL_VELOCITY.x);
  }

  // Update each star
  for (size_t i = 0; i < list_size(state->stars) - (star_just_spawned ? 1 : 0);
       ++i) {
    body_t *star = list_get(state->stars, i);
    special_state_t previous_special_state =
        get_special_state(body_get_shape(star));

    // Update vertex positions (only apply gravity if not in ground)
    vector_t star_grav_acc =
        (previous_special_state == IN_GROUND) ? VEC_ZERO : GRAV_ACC;
    body_evolve(star, dt, star_grav_acc, 0.0);

    special_state_t vertices_special_state =
        get_special_state(body_get_shape(star));
    switch (vertices_special_state) {
    case OFF_SCREEN:
      // Remove the star
      list_remove(state->stars, i);
      body_free(star);
      i -= 1;
      break;
    case IN_GROUND:
      // Bounce the star back up
      if (previous_special_state == IN_GROUND) {
        // The star was already bounced. Only need to ensure it won't fall
        // through.
        vector_t curr_velocity = body_get_velocity(star);
        vector_t new_velocity = {.x = curr_velocity.x,
                                 .y = fmax(curr_velocity.y, BOUNCE_CORRECTION)};
        body_set_velocity(star, new_velocity);
        break;
      }
      if (body_get_velocity(star).y >= 0) {
        // The star rotated and hit the ground again due to low velocity. Give
        // it a small boost.
        vector_t curr_velocity = body_get_velocity(star);
        vector_t new_velocity = {.x = curr_velocity.x,
                                 .y = curr_velocity.y + BOUNCE_CORRECTION};
        body_set_velocity(star, new_velocity);
      } else {
        // Normal bounce
        vector_t curr_velocity = body_get_velocity(star);
        vector_t new_velocity = {.x = curr_velocity.x,
                                 .y = fabs(curr_velocity.y) * get_elasticity()};
        body_set_velocity(star, new_velocity);
      }
      break;
    case NO_SPECIAL_STATE:
      break;
    }
  }

  list_t *ground = make_ground();

  sdl_clear();
  sdl_draw_polygon(ground, GROUND_COLOR);
  for (size_t i = 0; i < list_size(state->stars); ++i) {
    body_t *star = list_get(state->stars, i);
    sdl_draw_polygon(body_get_shape(star), body_get_color(star));
  }
  sdl_show();
}

void emscripten_free(state_t *state) {
  list_free(state->stars);
  free(state);
}
