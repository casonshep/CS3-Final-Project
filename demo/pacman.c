#include "body.h"
#include "color.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const double TWO_PI = 2.0 * M_PI;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// Pacman Constants
const double PACMAN_RADIUS = 40;
const double PACMAN_MOUTH_ANGLE = (double)11 / 72 * TWO_PI;

// Pebble constants
const size_t CIRCLE_POINTS = 40;
const double PEBBLE_RADIUS = 5;
const size_t INITIAL_NUM_PEBBLES = 12;
const double SPAWN_DELAY = 15;

// Evolution constants
const double INITIAL_VELOCITY = 150;
const double ACCELERATION = 500;

// Color constants
const rgb_color_t PACMAN_COLOR = COLOR_YELLOW;
const rgb_color_t PEBBLE_COLOR = COLOR_YELLOW;

// Compatibility
const double MASS = 1;

/**
 * Generates Pebble vertices list given radius of Pebble, number of vertices
 * of Pebble, and center coordinates (vector_t) of Pebble.
 * Returns list_tof vertices of Pebble.
 */
list_t *make_pebble(size_t radius, size_t num_points, vector_t center) {
  // Initialize angles and pebble
  double curr_angle = 0;
  double vert_angle = TWO_PI / num_points;
  list_t *vertices = list_init(num_points, free);
  assert(vertices != NULL);

  // Add vertices to pebble
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

/**
 * Generates Pacman vertices list given radius of Pacman, number of vertices
 * of Pacman, and center coordinates (vector_t) of Pacman.
 * Returns list_t of vertices of Pacman.
 */
list_t *make_pacman(size_t radius, size_t num_points, vector_t center) {
  // Initialize angles and Pacman polygon
  size_t num_points_skipped = PACMAN_MOUTH_ANGLE / TWO_PI * num_points;
  double vert_angle = TWO_PI / num_points;
  double curr_angle = PACMAN_MOUTH_ANGLE / 2;
  size_t num_points_drawn =
      num_points - num_points_skipped + 1; // doesn't include center
  list_t *vertices = list_init(num_points_drawn, free);
  assert(vertices != NULL);

  // Add center vertex
  vector_t *malloced_center = malloc(sizeof(vector_t));
  malloced_center->x = center.x;
  malloced_center->y = center.y;
  list_add(vertices, malloced_center);

  for (size_t i = 0; i < num_points_drawn; i++) {
    // Calculate Cartesian coordinates
    double x = cos(curr_angle) * radius + center.x;
    double y = sin(curr_angle) * radius + center.y;

    // Create vertex
    vector_t *vec_ptr = malloc(sizeof(vector_t));
    vec_ptr->x = x;
    vec_ptr->y = y;

    // Add vertex and increment angle
    list_add(vertices, vec_ptr);
    curr_angle += vert_angle;
  }
  return vertices;
}

typedef enum { NO_WALL, LEFT_WALL, TOP_WALL, RIGHT_WALL, BOTTOM_WALL } wall_t;

/**
 * Detects which wall the given polygon is COMPLETELY inside of, if any.
 */
wall_t inside_wall(list_t *vertices) {
  size_t num_vertices = list_size(vertices);

  bool in_left_wall = true;
  bool in_top_wall = true;
  bool in_right_wall = true;
  bool in_bottom_wall = true;

  for (size_t i = 0; i < num_vertices; i++) {
    vector_t *vertex = list_get(vertices, i);

    in_left_wall &= vertex->x <= 0;
    in_top_wall &= vertex->y >= WINDOW.y;
    in_right_wall &= vertex->x >= WINDOW.x;
    in_bottom_wall &= vertex->y <= 0;
  }

  return in_left_wall
             ? LEFT_WALL
             : (in_top_wall ? TOP_WALL
                            : (in_right_wall
                                   ? RIGHT_WALL
                                   : (in_bottom_wall ? BOTTOM_WALL : NO_WALL)));
}

/**
 * Generates a random center coordinate to generate a Pebble at.
 */
vector_t random_loc() {
  return (vector_t){.x = (double)rand() / RAND_MAX * WINDOW.x,
                    .y = (double)rand() / RAND_MAX * WINDOW.y};
}

/**
 * Returns index of closest Pebble to Pacman in bodies list. Takes in
 * coordinates of center of Pacman to avoid and list of bodies to iterate
 * through. Returns 0 if there are no pebbles.
 */
size_t closest_pebble(vector_t pacman_center, scene_t *scene) {
  if (scene_bodies(scene) == 1) {
    // No pebbles
    return 0;
  }

  double min_dist = INFINITY;
  size_t min_idx = 0;
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    if (vec_dist(pacman_center, body_get_centroid(curr_body)) < min_dist) {
      min_dist = vec_dist(pacman_center, body_get_centroid(curr_body));
      min_idx = i;
    }
  }
  return min_idx;
}

struct state {
  scene_t *scene;
  double time_until_next_pebble;
};

/**
 * Deletes the cosest Pebble to Pacman if its center is less than
 * Pacman's radius away from Pacman's center.
 */
void handle_pebble_eating(state_t *state) {
  scene_t *scene = state->scene;
  body_t *pacman = scene_get_body(scene, 0);
  vector_t pacman_center = body_get_centroid(pacman);

  size_t closest_pebble_idx = closest_pebble(pacman_center, scene);
  if (closest_pebble_idx == 0) {
    // No pebbles
    return;
  }
  body_t *closest_pebble_body = scene_get_body(scene, closest_pebble_idx);
  vector_t closest_pebble_center = body_get_centroid(closest_pebble_body);

  if (vec_dist(pacman_center, closest_pebble_center) <= PACMAN_RADIUS) {
    scene_remove_body(scene, closest_pebble_idx);
  }
}

/**
 * Generates pebbles in the scene in random time intervals between
 * [0, SPAWN_DELAY] (seconds) inclusive.
 */
void handle_pebble_spawning(state_t *state) {
  if (state->time_until_next_pebble <= 0) {
    body_t *pebble =
        body_init(make_pebble(PEBBLE_RADIUS, CIRCLE_POINTS, random_loc()), MASS,
                  PEBBLE_COLOR);
    scene_add_body(state->scene, pebble);
    state->time_until_next_pebble = SPAWN_DELAY * (double)rand() / RAND_MAX;
  }
}

/**
 * Moves Pacman to the opposite side of the screen if he is inside a wall
 */
void handle_wrap_around(state_t *state) {
  body_t *pacman = scene_get_body(state->scene, 0);
  vector_t curr_centroid = body_get_centroid(pacman);
  double curr_x = curr_centroid.x;
  double curr_y = curr_centroid.y;

  vector_t new_centroid;
  switch (inside_wall(body_get_shape(pacman))) {
  case LEFT_WALL:
    new_centroid = (vector_t){WINDOW.x, curr_y};
    break;
  case TOP_WALL:
    new_centroid = (vector_t){curr_x, 0};
    break;
  case RIGHT_WALL:
    new_centroid = (vector_t){0, curr_y};
    break;
  case BOTTOM_WALL:
    new_centroid = (vector_t){curr_x, WINDOW.y};
    break;
  case NO_WALL:
    return;
  }

  body_set_centroid(pacman, new_centroid);
}

/**
 * Keypress handler to perform rotation/translation based on duration pressed.
 */
void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *pacman = scene_get_body(state->scene, 0);
  double current_speed = INITIAL_VELOCITY + ACCELERATION * held_time;

  if (type == KEY_PRESSED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_rotation(pacman, 0.5 * TWO_PI);
      body_set_velocity(pacman, (vector_t){-(current_speed), 0});
      break;
    case UP_ARROW:
      body_set_rotation(pacman, 0.25 * TWO_PI);
      body_set_velocity(pacman, (vector_t){0, current_speed});
      break;
    case RIGHT_ARROW:
      body_set_rotation(pacman, 0);
      body_set_velocity(pacman, (vector_t){(current_speed), 0});
      break;
    case DOWN_ARROW:
      body_set_rotation(pacman, 0.75 * TWO_PI);
      body_set_velocity(pacman, (vector_t){0, -(current_speed)});
      break;
    }
  }
  if (type == KEY_RELEASED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(pacman, (vector_t){0, 0});
      break;
    case UP_ARROW:
      body_set_velocity(pacman, (vector_t){0, 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(pacman, (vector_t){0, 0});
      break;
    case DOWN_ARROW:
      body_set_velocity(pacman, (vector_t){0, 0});
      break;
    }
  }
}

state_t *emscripten_init() {
  srand(time(NULL));

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;
  state->time_until_next_pebble = 0;

  // Initialize window and renderer
  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);
  sdl_on_key((void *)on_key);

  // Create Pacman and add to bodies in scene
  body_t *pacman = body_init(make_pacman(PACMAN_RADIUS, CIRCLE_POINTS, CENTER),
                             MASS, PACMAN_COLOR);
  scene_add_body(state->scene, pacman);

  // Create Pebbles at random locations and add
  for (size_t i = 0; i < INITIAL_NUM_PEBBLES; i++) {
    body_t *pebble =
        body_init(make_pebble(PEBBLE_RADIUS, CIRCLE_POINTS, random_loc()), MASS,
                  PEBBLE_COLOR);
    scene_add_body(state->scene, pebble);
  }
  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  scene_t *scene = state->scene;
  scene_tick(scene, dt);
  state->time_until_next_pebble -= dt;

  handle_pebble_eating(state);
  handle_pebble_spawning(state);
  handle_wrap_around(state);

  sdl_render_scene(scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}
