#include "color.h"
#include "forces.h"
#include "list.h"
#include "player.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

// Mathematical Constants
const double TWO_PI = 2.0 * M_PI;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// General constants
const size_t ARBITRARY_MASS = 1;
const size_t CIRCLE_POINTS = 40;

// Landscape constants
const double HEXAGON_RADIUS = 50;
const double LEFT_BOUNDARY = 350;
const double RIGHT_BOUNDARY = 650;
const size_t HEXAGON_POINTS = 6;

// Border constants
const double BORDER_WIDTH = 10;

// Player Constants
const double PLAYER_SIZE = 30;
const double PLAYER_SPEED = 250;
const double PLAYER_MASS = 2;
const double DRAG_COEFF = 2;
const size_t INITIAL_HEALTH = 3;
const vector_t PLAYER1_CENTER = {.x = 250, .y = 250};
const vector_t PLAYER2_CENTER = {.x = 750, .y = 250};
const rgb_color_t PLAYER1_COLOR = COLOR_BLUE;
const rgb_color_t PLAYER2_COLOR = COLOR_RED;
const size_t BASE_SHOT_COUNT = 1;

// Trajectory constants
const size_t NUM_DOTS = 15;

// Aim constants
const double AIMING_SPEED = 30;
const double IMPULSE_PROBABILITY = .3;
const double IMPULSE_MAX = 10;

// Powerup Constants
const double POWERUP_RADIUS = 5;
const double SPAWN_DELAY = 45;
const rgb_color_t POWERUP_COLOR = COLOR_YELLOW;
const rgb_color_t SHIELD_COLOR = COLOR_AQUA;
const size_t REGEN_AMOUNT = 3;
const size_t SHIELD_RADIUS = 40;

// Health Bar Constants
const double HEALTH_BAR_LENGTH = 100;
const double HEALTH_BAR_HEIGHT = 25;
const vector_t HEALTH_BAR_1_POS = {30, 30};
const vector_t HEALTH_BAR_2_POS = {870, 30};
const rgb_color_t HEALTH_BAR_COLOR = {1, 0.75, 0.8};

struct state {
  scene_t *scene;
  double powerup_spawn_delay;
  size_t active_player;
  vector_t aim_center;
  size_t shots_left;
  time_t time;
  time_t countdown;
  bool game_over;
};

typedef enum body_type {
  TRAJECTORY,
  BACKGROUND,
  LANDSCAPE,
  BORDER,
  PLAYER,
  BULLET,
  POWERUP1, // Player 1's active powerup
  POWERUP2, // Player 2's active powerup
  HEALTH1,  // Player 1's health bar
  HEALTH2,  // Player 2's health bar
  SHIELD_BODY
} body_type_t;

typedef enum powerup_type {
  EXTRA_SHOT,
  REGENERATION,
  SHIELD,
  ALL_OR_NOTHING,
  NONE
} powerup_type_t;

typedef struct {
  body_type_t type;
  size_t health;
  powerup_type_t powerup;
} body_info_t;

/**
 * Creates body info based on given body type
 * To be passed to body_init_with_info with free as its freer.
 */
body_info_t *create_general_info(body_type_t body_type) {
  body_info_t *info = malloc(sizeof(body_info_t));
  info->type = body_type;
  return info;
}

/**
 * Creates Player body info based on given health
 * To be passed to body_init_with_info with free as its freer.
 */
body_info_t *create_player_info(size_t health) {
  body_info_t *info = malloc(sizeof(body_info_t));
  info->type = PLAYER;
  info->health = health;
  return info;
}

/**
 * Creates Powerup body info based on given powerup type.
 * To be passed to body_init_with_info with free as its freer.
 */
body_info_t *create_powerup_info(powerup_type_t powerup_type,
                                 body_type_t body_type) {
  body_info_t *info = malloc(sizeof(body_info_t));
  info->type = body_type;
  info->powerup = powerup_type;
  return info;
}

/**
 * Generates Circle vertices list given radius of Circle and center coordinates
 * (vector_t) of Circle.
 */
list_t *make_circle(double radius, vector_t center) {
  list_t *circle = list_init(CIRCLE_POINTS, free);
  double arc_angle = TWO_PI / CIRCLE_POINTS;
  assert(circle != NULL);

  for (size_t i = 0; i < CIRCLE_POINTS; ++i) {
    // Calculate polar coordinates relative to center
    double angle = i * arc_angle;

    // Calculate cartesian coordinates relative to (0, 0)
    double x = cos(angle) * radius + center.x;
    double y = sin(angle) * radius + center.y;

    // Create and add vertex
    vector_t *vertex = malloc(sizeof(vector_t));
    vertex->x = x;
    vertex->y = y;
    list_add(circle, vertex);
  }

  return circle;
}

/**
 * Generates Rectangle vertices list given length and height of Rectangle,
 * and center coordinates (vector_t) of Rectangle.
 * Returns list_t of vertices of Rectangle.
 */
list_t *make_rectangle(double length, double height, vector_t center) {
  vector_t *bl = malloc(sizeof(vector_t));
  vector_t *br = malloc(sizeof(vector_t));
  vector_t *tr = malloc(sizeof(vector_t));
  vector_t *tl = malloc(sizeof(vector_t));

  vector_t tr_disp = {length / 2, height / 2};
  vector_t tl_disp = {-1 * length / 2, height / 2};
  *bl = (vector_t)vec_subtract(center, tr_disp);
  *br = (vector_t)vec_subtract(center, tl_disp);
  *tr = (vector_t)vec_add(center, tr_disp);
  *tl = (vector_t)vec_add(center, tl_disp);

  list_t *vertices = list_init(4, free);
  assert(vertices != NULL);

  list_add(vertices, bl);
  list_add(vertices, br);
  list_add(vertices, tr);
  list_add(vertices, tl);

  return vertices;
}

/**
 * Returns a hexagon. Relative to the center, vertices are spaced at equal
 * angles.
 */
list_t *make_hexagon(double radius, vector_t center) {
  double vertex_angle = TWO_PI / HEXAGON_POINTS;
  list_t *vertices = list_init(HEXAGON_POINTS, free);
  assert(vertices != NULL);

  for (size_t i = 0; i < HEXAGON_POINTS; ++i) {
    // Calculate polar coordinates relative to center
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
 * Returns a list of all of the bodies in the scene of the given type in the
 * order that they were added to the scene.
 */
list_t *get_bodies_by_type(state_t *state, body_type_t searching) {
  list_t *bodies = list_init(1, (free_func_t)body_free);
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    body_t *body = scene_get_body(state->scene, i);
    body_info_t info = *(body_info_t *)body_get_info(body);
    if (info.type == searching) {
      list_add(bodies, body);
    }
  }
  return bodies;
}

/**
 * Returns whether two colors are equal.
 */
bool colors_are_equal(rgb_color_t color1, rgb_color_t color2) {
  return ((color1.r == color2.r) && (color1.g == color2.g) &&
          (color1.b == color2.b));
}

/**
 * Returns a list of the possible landscape tile colors (excluding white, i.e.
 * no landscape), in the order from darkest (most shots required to destroy) to
 * lightest (one shot required to destroy).
 */
list_t *landscape_colors_list() {
  size_t num_colors = 3;
  list_t *color_list = list_init(num_colors, free);

  rgb_color_t *dark_green = malloc(sizeof(rgb_color_t));
  *dark_green = (rgb_color_t){0.0 / 255, 158.0 / 255, 96.0 / 255};
  list_add(color_list, dark_green);

  rgb_color_t *mid_green = malloc(sizeof(rgb_color_t));
  *mid_green = (rgb_color_t){80.0 / 255, 200.0 / 255, 120.0 / 255};
  list_add(color_list, mid_green);

  rgb_color_t *light_green = malloc(sizeof(rgb_color_t));
  *light_green = (rgb_color_t){152.0 / 255, 251.0 / 255, 152.0 / 255};
  list_add(color_list, light_green);

  return color_list;
}

/**
 * Creates a shield around a player to be updated from handle_shield each tick.
 */
void create_shield(scene_t *scene, body_t *player) {
  vector_t center = body_get_centroid(player);
  list_t *vertices = make_circle(SHIELD_RADIUS, center);
  body_t *sheild = body_init_with_info(vertices, ARBITRARY_MASS, SHIELD_COLOR,
                                       create_general_info(SHIELD_BODY), free);
  scene_add_body(scene, sheild);
}

/**
 * Snaps each player's shield to its current position.
 */
void handle_shield(state_t *state) {
  list_t *shields = get_bodies_by_type(state, SHIELD_BODY);
}

/**
 * Updates health bars for the given player to match their current health. Does
 * nothing if their health is zero.
 */
void handle_player_health_display(state_t *state, body_t *player,
                                  body_type_t health_type,
                                  vector_t health_bar_pos) {
  body_info_t info = *(body_info_t *)body_get_info(player);
  size_t health = info.health;
  if (health <= 0) {
    return;
  }
  list_t *chunks = get_bodies_by_type(state, health_type);
  size_t num_chunks = list_size(chunks);
  if (num_chunks == health) {
    return;
  }
  if (num_chunks > health) {
    for (int i = num_chunks - 1; i >= health; --i) {
      body_remove(list_get(chunks, i));
    }
    return;
  }
  double chunk_length = HEALTH_BAR_LENGTH / INITIAL_HEALTH;
  for (size_t i = num_chunks; i < health; ++i) {
    vector_t chunk_center = {health_bar_pos.x + chunk_length * i,
                             health_bar_pos.y};
    list_t *chunk_verts =
        make_rectangle(chunk_length, HEALTH_BAR_HEIGHT, chunk_center);
    body_t *chunk = body_init_with_info(chunk_verts, INFINITY, HEALTH_BAR_COLOR,
                                        create_general_info(health_type), free);
    scene_add_body(state->scene, chunk);
  }
}

/**
 * Updates health bars for both players.
 */
void handle_health_display(state_t *state) {
  list_t *players = get_bodies_by_type(state, PLAYER);
  handle_player_health_display(state, list_get(players, 0), HEALTH1,
                               HEALTH_BAR_1_POS);
  handle_player_health_display(state, list_get(players, 1), HEALTH2,
                               HEALTH_BAR_2_POS);
}

/**
 * Event handler for a shell colliding with a player.
 */
void apply_shot_player_collision(body_t *shell, body_t *player, vector_t axis,
                                 void *aux) {
  body_info_t *player_info_ptr = (body_info_t *)body_get_info(player);
  player_info_ptr->health--;
  body_add_impulse(player, vec_multiply(1 / body_get_mass(player),
                                        body_get_velocity(shell)));
  body_remove(shell);
}

/**
 * Creates a collision handler between a shell and a player.
 */
void create_shot_player_collision(scene_t *scene, body_t *shell,
                                  body_t *player) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, shell);
  list_add(aux_bodies, player);

  list_t *consts = list_init(0, free);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  create_collision(scene, shell, player,
                   (collision_handler_t)apply_shot_player_collision, aux,
                   const_body_aux_free);
}

/**
 * Event handler for a player moving into a powerup.
 */
void apply_player_powerup_collision(scene_t *scene, body_t *player,
                                    body_t *powerup, vector_t axis, void *aux) {
  // get powerup type info
  body_info_t powerup_info = *(body_info_t *)body_get_info(powerup);
  // get player info
  body_info_t player_info = *(body_info_t *)body_get_info(player);

  switch (powerup_info.powerup) {
  case EXTRA_SHOT:
    // this will give powerup on the next turn
    player_info.powerup = EXTRA_SHOT;
    break;
  case REGENERATION:
    player_info.health += REGEN_AMOUNT;
    if (player_info.health > INITIAL_HEALTH) {
      player_info.health = INITIAL_HEALTH;
    }
    break;
  case SHIELD:
    // HANDLE SHEILD CREATION
    player_info.powerup = SHIELD;
    create_shield(scene, player);
    break;
  case ALL_OR_NOTHING:
    // HANDLE ALL OR NOTHING
    player_info.powerup = ALL_OR_NOTHING;
    break;
  case NONE:
    break;
  }

  body_remove(powerup);
}

/**
 * Creates a collision handler between a player and a powerup.
 */
void create_powerup_player_collision(scene_t *scene, body_t *player,
                                     body_t *powerup) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, player);
  list_add(aux_bodies, powerup);

  list_t *consts = list_init(0, free);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  create_collision(scene, player, powerup,
                   (collision_handler_t)apply_player_powerup_collision, aux,
                   const_body_aux_free);
}

/**
 * Event handler for shell collisions from all-or-nothing shot
 */
void apply_all_or_nothing_shot(body_t *body1, body_t *body2, vector_t axis,
                               void *aux) {}

/**
 * Creates a collision handler for all-or-nothing shots between the shell and
 * player.
 */
void create_all_or_nothing_shot(scene_t *scene, body_t *body1, body_t *body2) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);

  list_t *consts = list_init(0, free);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_all_or_nothing_shot, aux,
                   const_body_aux_free);
}

void apply_player_landscape_collision(body_t *body1, body_t *body2,
                                      vector_t axis, void *aux) {

  body_set_velocity(body2, VEC_ZERO);

  vector_t hexagon_center = body_get_centroid(body1);
  vector_t player_center = body_get_centroid(body2);

  vector_t difference_vector = vec_subtract(player_center, hexagon_center);
  vector_t direction = vec_normalize(difference_vector);

  vector_t offset_hexagon =
      vec_multiply((HEXAGON_RADIUS), direction); /// 2 *sqrt(3)
  vector_t offset_player = vec_multiply((PLAYER_SIZE), direction);

  vector_t new_center =
      vec_add(hexagon_center, vec_add(offset_hexagon, offset_player));
  body_set_centroid(body2, new_center);
}

void create_player_landscape_collision(scene_t *scene, body_t *body1,
                                       body_t *body2) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);

  list_t *consts = list_init(0, free);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_player_landscape_collision, aux,
                   const_body_aux_free);
}

/**
 * Generates a random center coordinate to generate a Powerup at depending on
 * maximum x value (used to generate both left and right powerups).
 */
vector_t random_loc() {
  return (vector_t){.x = (double)rand() / RAND_MAX * WINDOW.x,
                    .y = ((double)rand() / RAND_MAX * WINDOW.y)};
}

/**
 * Generates a random index between 0 and given range.
 */
size_t random_index(size_t range) {
  return (size_t)((double)rand() / RAND_MAX * range);
}

/**
 * Initializes Power-Ups of varying types (denoted by color) on white landscape
 * tile centroids.
 */
void handle_powerup_spawning(state_t *state) {
  if (state->powerup_spawn_delay <= 0) {
    size_t rand_index = random_index(4);

    powerup_type_t powerup_type;
    switch (rand_index) {
    case 0:
      powerup_type = EXTRA_SHOT;
      break;
    case 1:
      powerup_type = REGENERATION;
      break;
    case 2:
      powerup_type = SHIELD;
    default:
      powerup_type = ALL_OR_NOTHING;
    }

    vector_t loc = random_loc();
    body_type_t body_type = (loc.x < WINDOW.x / 2) ? POWERUP1 : POWERUP2;
    body_t *powerup = body_init_with_info(
        make_circle(POWERUP_RADIUS, loc), ARBITRARY_MASS, POWERUP_COLOR,
        create_powerup_info(powerup_type, body_type), free);
    scene_add_body(state->scene, powerup);

    // Create collision for both players
    list_t *players = get_bodies_by_type(state, PLAYER);
    body_t *player1 = list_get(players, 0);
    body_t *player2 = list_get(players, 1);
    create_powerup_player_collision(state->scene, player1, powerup);
    create_powerup_player_collision(state->scene, player2, powerup);
    state->powerup_spawn_delay = SPAWN_DELAY * (double)rand() / RAND_MAX;
  }
}

/**
 * Initializes hexagonal tiles of varying heights for landscape.
 * Darker colors correspond to higher heights, three tiers above ground (white).
 */
void make_landscape(state_t *state) {
  // References
  size_t width = 2 * HEXAGON_RADIUS;
  size_t height = sqrt(3) * HEXAGON_RADIUS;

  // Spacers
  vector_t spacing_next_space = {2 * 0.75 * width, 0};
  vector_t spacing_next_row = {0, 0.5 * height};

  // Starting hexagons
  vector_t top_left_hexagon = {0.5 * width, 0};
  vector_t offset_left_hexagon = {-0.25 * width, 0};

  // Columns and rows
  size_t num_cols = WINDOW.x / width;
  size_t num_rows = (WINDOW.y / (0.5 * height)) + 1;

  // Make color list
  list_t *color_list = landscape_colors_list();

  // Generate hexagons in rows
  vector_t next = top_left_hexagon;
  vector_t restart = VEC_ZERO;

  for (size_t i = 0; i <= num_rows; i++) {
    for (size_t j = 0; j < num_cols; j++) {

      // initialize bricks, and set them to proper location
      list_t *vertices = make_hexagon(HEXAGON_RADIUS, VEC_ZERO);
      body_t *hexagon =
          body_init_with_info(vertices, ARBITRARY_MASS, COLOR_WHITE,
                              create_general_info(LANDSCAPE), free);
      vector_t current_coord_add = vec_multiply((double)j, spacing_next_space);
      vector_t current_coord = vec_add(next, current_coord_add);
      body_set_centroid(hexagon, current_coord);

      // TODO: Fix buggy implementation of colors
      if (current_coord.x > LEFT_BOUNDARY && current_coord.x < RIGHT_BOUNDARY) {
        size_t random_color_index = random_index(3);
        rgb_color_t random_green =
            *(rgb_color_t *)list_get(color_list, random_color_index);
        body_set_color(hexagon, random_green);
      }

      scene_add_body(state->scene, hexagon);
    }

    // Move up to next row
    if ((int)(i) % 2 == 0) {
      restart = offset_left_hexagon;
    } else {
      restart = top_left_hexagon;
    }
    next.x = restart.x;
    next.y = next.y + spacing_next_row.y;
  }

  list_free(color_list);
}

void make_border(state_t *state) {
  vector_t top_loc = {CENTER.x, WINDOW.y};
  vector_t bot_loc = {CENTER.x, 0};
  vector_t right_loc = {WINDOW.x, CENTER.y};
  vector_t left_loc = {0, CENTER.y};

  list_t *border_top_vert = make_rectangle(WINDOW.x, BORDER_WIDTH, top_loc);
  body_t *border_top =
      body_init_with_info(border_top_vert, ARBITRARY_MASS, PLAYER1_COLOR,
                          create_general_info(BORDER), free);
  scene_add_body(state->scene, border_top);

  list_t *border_bot_vert = make_rectangle(WINDOW.x, BORDER_WIDTH, bot_loc);
  body_t *border_bot =
      body_init_with_info(border_bot_vert, ARBITRARY_MASS, PLAYER1_COLOR,
                          create_general_info(BORDER), free);
  scene_add_body(state->scene, border_bot);

  list_t *border_left_vert = make_rectangle(BORDER_WIDTH, WINDOW.y, left_loc);
  body_t *border_left =
      body_init_with_info(border_left_vert, ARBITRARY_MASS, PLAYER1_COLOR,
                          create_general_info(BORDER), free);
  scene_add_body(state->scene, border_left);

  list_t *border_right_vert = make_rectangle(BORDER_WIDTH, WINDOW.y, right_loc);
  body_t *border_right =
      body_init_with_info(border_right_vert, ARBITRARY_MASS, PLAYER1_COLOR,
                          create_general_info(BORDER), free);
  scene_add_body(state->scene, border_right);
}

void make_players(state_t *state) {
  list_t *vertices = make_hexagon(PLAYER_SIZE, PLAYER1_CENTER);
  body_t *player1 =
      body_init_with_info(vertices, PLAYER_MASS, PLAYER1_COLOR,
                          create_player_info(INITIAL_HEALTH), free);
  scene_add_body(state->scene, player1);
  create_drag(state->scene, DRAG_COEFF, player1);

  list_t *vertices1 = make_hexagon(PLAYER_SIZE, PLAYER2_CENTER);
  body_t *player2 =
      body_init_with_info(vertices1, PLAYER_MASS, PLAYER2_COLOR,
                          create_player_info(INITIAL_HEALTH), free);
  scene_add_body(state->scene, player2);
  create_drag(state->scene, DRAG_COEFF, player2);
}

void turn_reset(state_t *state) {
  list_t *borders = get_bodies_by_type(state, BORDER);
  rgb_color_t curr_border_color;

  list_t *players = get_bodies_by_type(state, PLAYER);

  if (state->active_player == 0) {
    state->active_player = 1;
    curr_border_color = PLAYER2_COLOR;
  } else {
    state->active_player = 0;
    curr_border_color = PLAYER1_COLOR;
  }
  // update the border colors:
  for (size_t i = 0; i < list_size(borders); i++) {
    body_t *curr_border = list_get(borders, i);
    body_set_color(curr_border, curr_border_color);
  }
  state->shots_left = BASE_SHOT_COUNT;

  // increase shot count if extra shot powerup
  // TODO: Debug
  body_t *player = list_get(players, state->active_player);
  body_info_t player_info = *(body_info_t *)body_get_info(player);
  if (player_info.powerup == EXTRA_SHOT) {
    state->shots_left += 1;
    player_info.powerup = NONE;
  }

  state->aim_center = CENTER;
}

void trajectory_dots(state_t *state) {
  list_t *players = get_bodies_by_type(state, PLAYER);
  body_t *player =
      state->active_player == 1 ? list_get(players, 1) : list_get(players, 0);

  // get aim center
  vector_t end_point = state->aim_center;

  vector_t center_pt = body_get_centroid(player);

  vector_t slope_coords = vec_subtract(end_point, center_pt);

  // double slope = slope_coords.y / slope_coords.x;

  double dx = slope_coords.x;
  double dy = slope_coords.y;

  double x_increment = (dx / 10);
  double y_increment = (dy / 10);
  vector_t increment = {x_increment, y_increment};

  vector_t circle_coord = center_pt;
  for (size_t i = 0; i <= 10; i++) {
    list_t *circle = make_circle(1, circle_coord);
    body_t *dot = body_init_with_info(circle, 1, COLOR_BLACK,
                                      create_general_info(TRAJECTORY), free);
    body_set_centroid(dot, circle_coord);
    scene_add_body(state->scene, dot);
    circle_coord = vec_add(circle_coord, increment);
  }
  make_circle(1, end_point);
}

void shoot(body_t *shooting_body, state_t *state) {
  list_t *players = get_bodies_by_type(state, PLAYER);
  body_t *player = (body_t *)list_get(players, (state->active_player + 1) % 2);
  body_info_t player_info = *(body_info_t *)body_get_info(player);

  vector_t center = body_get_centroid(shooting_body);
  rgb_color_t color = body_get_color(shooting_body);

  list_t *vertices = make_circle(10, center);
  body_t *shot = body_init_with_info(vertices, INFINITY, color,
                                     create_general_info(BULLET), free);

  body_set_velocity(shot, vec_subtract(state->aim_center, center));

  // Add health decrement/recoil collision or ALL OR NOTHING COLLISION
  if (player_info.powerup == ALL_OR_NOTHING) {
    create_all_or_nothing_shot(state->scene, shot, player);
  } else {
    create_shot_player_collision(state->scene, shot, player);
  }

  // Add destructive collisions between shot and all existing invaders
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    body_t *body = scene_get_body(state->scene, i);
    body_info_t info = *(body_info_t *)body_get_info(body);
    if (info.type == BULLET || info.type == LANDSCAPE) {
      if (colors_are_equal(body_get_color(body), COLOR_WHITE) == 0) {
        create_color_increment_collision(state->scene, shot, body,
                                         landscape_colors_list(),
                                         (free_func_t)list_free);
      }
    }
  }
  create_random_impulse(state->scene, IMPULSE_PROBABILITY, IMPULSE_MAX, shot);
  scene_add_body(state->scene, shot);
}

void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  list_t *players = get_bodies_by_type(state, PLAYER);
  body_t *player =
      state->active_player == 1 ? list_get(players, 1) : list_get(players, 0);

  if (type == KEY_PRESSED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(player, (vector_t){-PLAYER_SPEED, 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(player, (vector_t){PLAYER_SPEED, 0});
      break;
    case UP_ARROW:
      body_set_velocity(player, (vector_t){0, PLAYER_SPEED});
      break;
    case DOWN_ARROW:
      body_set_velocity(player, (vector_t){0, -PLAYER_SPEED});
      break;
    case W:
      state->aim_center =
          vec_add(state->aim_center, (vector_t){0, AIMING_SPEED});
      break;
    case A:
      state->aim_center =
          vec_add(state->aim_center, (vector_t){-AIMING_SPEED, 0});
      break;
    case S:
      state->aim_center =
          vec_add(state->aim_center, (vector_t){0, -AIMING_SPEED});
      break;
    case D:
      state->aim_center =
          vec_add(state->aim_center, (vector_t){AIMING_SPEED, 0});
      break;
    case SPACE:
      if (state->shots_left > 0) {
        shoot(player, state);
        state->shots_left--;
      }
      break;
    }
  }
  if (type == KEY_RELEASED) {
    switch (key) {
    case LEFT_ARROW:
    case RIGHT_ARROW:
    case UP_ARROW:
    case DOWN_ARROW:
      body_set_velocity(player, VEC_ZERO);
      break;
    case SPACE:
      if (state->shots_left < 1) {
        turn_reset(state);
      }
      break;
    }
  }
}

void end_screen(state_t *state) {
  sdl_clear();
  list_t *window = make_rectangle(WINDOW.x, WINDOW.y, CENTER);
  body_t *background = body_init_with_info(
      window, PLAYER_MASS, COLOR_WHITE, create_general_info(BACKGROUND), free);
  scene_add_body(state->scene, background);
  list_t *players = get_bodies_by_type(state, PLAYER);

  body_info_t player1_info =
      *(body_info_t *)body_get_info(list_get(players, 0));
  body_info_t player2_info =
      *(body_info_t *)body_get_info(list_get(players, 1));
  /*if (body_is_removed(list_get(players, 0))) {
    draw_text(50, (size_t)CENTER.x - 125, (size_t)CENTER.y - 60, 200, 80,
              "Player 2 has won!");
  } else if (body_is_removed(list_get(players, 1))) {
    draw_text(50, (size_t)CENTER.x - 125, (size_t)CENTER.y - 60, 200, 80,
              "Player 1 has won!");
  } else*/
  if (player2_info.health > player1_info.health) {
    draw_text(50, (size_t)CENTER.x - 125, (size_t)CENTER.y - 60, 400, 80,
              "Player 2 has won!");
  } else if (player2_info.health < player1_info.health) {
    draw_text(50, (size_t)CENTER.x - 125, (size_t)CENTER.y - 60, 400, 80,
              "Player 1 has won!");
  } else {
    draw_text(50, (size_t)CENTER.x - 125, (size_t)CENTER.y - 60, 400, 80,
              "It's a tie! Boo!");
  }
}

bool handle_end_screen(state_t *state) {
  time_t countdown = state->countdown;
  list_t *players = get_bodies_by_type(state, PLAYER);
  body_t *player1 = list_get(players, 0);
  body_t *player2 = list_get(players, 1);
  body_info_t info1 = *(body_info_t *)body_get_info(player1);
  body_info_t info2 = *(body_info_t *)body_get_info(player2);
  if (info1.health <= 0 || info2.health <= 0 || countdown <= 0) {
    end_screen(state);
    return true;
  }
  return false;
}

state_t *emscripten_init() {
  srand(time(NULL));

  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);
  sdl_on_key((void *)on_key);

  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->active_player = 0;
  state->scene = scene;
  state->aim_center = CENTER;
  state->shots_left = BASE_SHOT_COUNT;
  state->game_over = false;

  // ORDER MATTERS HERE:
  // make_background(state); // TODO: Do we even need or want a background
  // still?
  make_landscape(state);
  make_players(state);
  make_border(state);

  // Get players for landscape collision
  list_t *players = get_bodies_by_type(state, PLAYER);
  list_t *hexagons = get_bodies_by_type(state, LANDSCAPE);

  body_t *player1 = (body_t *)list_get(players, 0);
  body_t *player2 = (body_t *)list_get(players, 1);

  for (size_t i = 0; i < list_size(hexagons); i++) {
    body_t *hexagon = (body_t *)list_get(hexagons, i);
    if (colors_are_equal(body_get_color(hexagon), COLOR_WHITE) == 0) {
      create_player_landscape_collision(state->scene, hexagon, player1);
      create_player_landscape_collision(state->scene, hexagon, player2);
    }
  }

  time_t countdown = 300000;
  state->countdown = countdown;

  // keep_on_screen(state); // TODO: might be useful to keep players on screen
  // make_display(state); // TODO: this will display health, powerups, other
  // game info

  return state;
}

void emscripten_main(state_t *state) {
  if (state->game_over) {
    return;
  }
  double dt = time_since_last_tick();

  scene_t *scene = state->scene;
  time_t countdown = state->countdown;
  state->powerup_spawn_delay -= dt;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (*(body_type_t *)body_get_info(body) == TRAJECTORY) {
      body_remove(body);
    }
  }
  scene_tick(scene, dt);
  trajectory_dots(state);
  handle_powerup_spawning(state);
  handle_health_display(state);
  sdl_render_scene(scene);
  display_clock(countdown);
  state->countdown -= (dt * 1000);
  state->game_over = handle_end_screen(state);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}