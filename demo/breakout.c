#include "color.h"
#include "forces.h"
#include "list.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Mathematical Constants
const double TWO_PI = 2.0 * M_PI;

// Window constants
const vector_t WINDOW = {.x = 1000, .y = 500};
const vector_t CENTER = {.x = 500, .y = 250};

// Player constants
const double PLAYER_WIDTH = 50;
const double PLAYER_HEIGHT = 20;
const rgb_color_t PLAYER_COLOR = COLOR_RED;
const double PLAYER_MASS = INFINITY;
const vector_t PLAYER_INIT_POSITION = {.x = 500, .y = 15};
const vector_t LEFTMOST_PLAYER_CENTROID = {.x = 175, .y = 15};
const vector_t RIGHTMOST_PLAYER_CENTROID = {.x = 825, .y = 15};
const double PLAYER_SPEED = 350;

// Border constants
const size_t BORDER_LENGTH = 150;
const vector_t BORDER1_CENTER = {.x = 75, .y = 250};
const vector_t BORDER2_CENTER = {.x = 925, .y = 250};
const rgb_color_t BORDER_COLOR = COLOR_BLACK;

// Ball constants
const size_t CIRCLE_POINTS = 40;
const vector_t START_VELOCITY = {.x = 150.0, .y = 200.0};
const double BALL_RADIUS = 5.0;
const double BALL_MASS = 1.0;
const rgb_color_t BALL_COLOR = {1, 0, 0};
const vector_t BALL_INIT_CENTER = {.x = 500, .y = 40};
const double ELASTICITY_CONST = 1.0;

// Brick generation constants
const size_t BRICK_LENGTH = 61;
const size_t BRICK_HEIGHT = 30;
const size_t BRICK_MASS = INFINITY;
const size_t NUM_COLS = 10;
const size_t NUM_ROWS = 3;
const size_t ROW_SPACING = 8;
const size_t COL_SPACING = 8;
const vector_t TOP_LEFT_BRICK = {190, 477.4};

// Powerup constants
const double POWERUP_SPAWN_DELAY = 30;
const double BOOST_FACTOR = 1.1;
const double POWERUP_RADIUS = 10;

typedef enum { BALL, FROZEN, WALL, GRAVITY } body_type_t;

body_type_t *make_type_info(body_type_t type) {
  body_type_t *info = malloc(sizeof(*info));
  *info = type;
  return info;
}

body_type_t get_type(body_t *body) {
  return *(body_type_t *)body_get_info(body);
}

list_t *colors_list() {
  list_t *color_list = list_init(NUM_COLS, free);
  for (size_t i = 0; i < NUM_COLS; ++i) {
    rgb_color_t *color = malloc(sizeof(rgb_color_t));
    *color = color_from_hsv(TWO_PI * i / NUM_COLS, 1, 1);
    list_add(color_list, color);
  }
  return color_list;
}

/**
 * Generates random location within the play area for powerups to spawn.
 */
vector_t random_powerup_loc() {
  // using demo parameters to define area for powerups to spawn
  double min_x = BORDER1_CENTER.x + BORDER_LENGTH / 2 + POWERUP_RADIUS / 2;
  double max_x = BORDER2_CENTER.x - BORDER_LENGTH / 2 - POWERUP_RADIUS / 2;

  double min_y = PLAYER_INIT_POSITION.y + PLAYER_HEIGHT / 2 + POWERUP_RADIUS;
  double max_y = TOP_LEFT_BRICK.y - (NUM_ROWS - .5) * BRICK_HEIGHT -
                 ROW_SPACING * (NUM_ROWS - 1) - POWERUP_RADIUS / 2;

  double x = (double)rand() / RAND_MAX * (max_x - min_x) + min_x;
  double y = (double)rand() / RAND_MAX * (max_y - min_y) + min_y;

  return (vector_t){x, y};
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
 * Generates Circle with given radius.
 */
list_t *circle_init(double radius) {
  list_t *circle = list_init(CIRCLE_POINTS, free);
  double arc_angle = TWO_PI / CIRCLE_POINTS;
  vector_t point = {.x = radius, .y = 0.0};
  for (size_t i = 0; i < CIRCLE_POINTS; i++) {
    vector_t *v = malloc(sizeof(*v));
    *v = point;
    list_add(circle, v);
    point = vec_rotate(point, arc_angle);
  }
  return circle;
}

/** Creates a ball with the given starting position and velocity */
body_t *get_ball(vector_t center, vector_t velocity) {
  list_t *shape = circle_init(BALL_RADIUS);
  body_t *ball = body_init_with_info(shape, BALL_MASS, BALL_COLOR,
                                     make_type_info(BALL), free);
  body_set_centroid(ball, center);
  body_set_velocity(ball, velocity);

  return ball;
}

struct state {
  scene_t *scene;
  double powerup_spawn_delay;
};

typedef enum special_state {
  NO_SPECIAL_STATE,
  OFF_SCREEN_BOTTOM,
  OFF_SCREEN_TOP,
  TOUCHING_RIGHT,
  TOUCHING_LEFT
} special_state_t;

/**
 * Returns OFF_SCREEN_BOTTOM if the shape is completely past the bottom
 * of the screen, OFF_SCREEN_TOP if the shape is past the top of the screen,
 * TOUCHING_RIGHT if the shape is touching the right border,
 * TOUCHING_LEFT if the shape is touching the left border,
 * and NO_SPECIAL_STATE otherwise.
 */
special_state_t get_special_state(body_t *body) {
  vector_t center = body_get_centroid(body);
  // checks for player
  if (body_get_mass(body) == INFINITY) {
    if (center.x - PLAYER_WIDTH / 2 < BORDER1_CENTER.x + BORDER_LENGTH / 2) {
      return TOUCHING_LEFT;
    } else if (center.x + PLAYER_WIDTH / 2 >
               BORDER2_CENTER.x - BORDER_LENGTH / 2) {
      return TOUCHING_RIGHT;
    }
  } else {
    // checks for ball
    if (center.y < 0) {
      return OFF_SCREEN_BOTTOM;
    } else if (center.y > WINDOW.y) {
      return OFF_SCREEN_TOP;
    } else if (center.x <=
               BORDER1_CENTER.x + BORDER_LENGTH / 2 + BALL_RADIUS / 2) {
      return TOUCHING_LEFT;
    } else if (center.x >=
               BORDER2_CENTER.x - BORDER_LENGTH / 2 - BALL_RADIUS / 2) {
      return TOUCHING_RIGHT;
    }
  }
  return NO_SPECIAL_STATE;
}

/**
 * Resets all the objects and forces in the scene.
 * Used to reset the game when the ball touches the
 * bottom of the screen.
 * Also called on game initialization.
 */
void reset_screen(state_t *state) {
  state->powerup_spawn_delay = POWERUP_SPAWN_DELAY;

  scene_t *scene = scene_init();
  state->scene = scene;

  // Create Border, three black rectangles
  list_t *pts1 = make_rectangle(BORDER_LENGTH, WINDOW.y, BORDER1_CENTER);
  body_t *border1 = body_init(pts1, INFINITY, BORDER_COLOR);
  scene_add_body(state->scene, border1);

  list_t *pts2 = make_rectangle(BORDER_LENGTH, WINDOW.y, BORDER2_CENTER);
  body_t *border2 = body_init(pts2, INFINITY, BORDER_COLOR);
  scene_add_body(state->scene, border2);

  // Create Player and add to the scene
  list_t *vertices =
      make_rectangle(PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_INIT_POSITION);
  body_t *player = body_init(vertices, PLAYER_MASS, PLAYER_COLOR);
  scene_add_body(state->scene, player);

  // Create ball and add to the screen
  body_t *ball = get_ball(BALL_INIT_CENTER, START_VELOCITY);
  scene_add_body(state->scene, ball);

  // Create physics collision between ball and the player
  create_physics_collision(state->scene, ELASTICITY_CONST, ball, player);

  // GENERATE BRICKS IN ROWS
  vector_t spacing_nextrow = {0, BRICK_HEIGHT + ROW_SPACING};
  spacing_nextrow = vec_negate(spacing_nextrow);
  vector_t spacing_nextspace = {BRICK_LENGTH + COL_SPACING, 0};
  // make color list
  list_t *color_list = colors_list();
  vector_t next = TOP_LEFT_BRICK;

  for (size_t i = 1; i <= NUM_ROWS; i++) {
    for (size_t j = 0; j < NUM_COLS; j++) {

      // initialize bricks, and set them to proper location
      list_t *pts = make_rectangle(BRICK_LENGTH, BRICK_HEIGHT, VEC_ZERO);
      body_t *rect =
          body_init(pts, BRICK_MASS, *(rgb_color_t *)list_get(color_list, j));
      vector_t current_coordadd = vec_multiply((double)j, spacing_nextspace);
      vector_t current_coord = vec_add(next, current_coordadd);
      body_set_centroid(rect, current_coord);

      // create the collision handlers for the ball and bricks
      create_single_destructive_collision(state->scene, rect, ball);
      create_physics_collision(state->scene, ELASTICITY_CONST, rect, ball);
      scene_add_body(state->scene, rect);
    }
    // move down to next row
    vector_t ydist_const = vec_multiply((double)i, spacing_nextrow);
    next = vec_add(TOP_LEFT_BRICK, ydist_const);
  }
  list_free(color_list);
}

/**
 * Handles the ball touching the left, right
 * or top border.
 */
void ball_within_walls(state_t *state) {
  body_t *ball = scene_get_body(state->scene, 3);
  special_state_t ball_special_state = get_special_state(ball);
  vector_t velocity = body_get_velocity(ball);
  switch (ball_special_state) {
  case OFF_SCREEN_BOTTOM:
    scene_free(state->scene);
    reset_screen(state);
    break;
  case OFF_SCREEN_TOP:
    body_set_velocity(ball, (vector_t){velocity.x, -1 * velocity.y});
    break;
  case TOUCHING_RIGHT:
    body_set_velocity(ball, (vector_t){-1 * velocity.x, velocity.y});
    break;
  case TOUCHING_LEFT:
    body_set_velocity(ball, (vector_t){-1 * velocity.x, velocity.y});
    break;
  case NO_SPECIAL_STATE:
    break;
  }
}

/**
 * Randomly spawns powerups based on the spawn delay.
 * In each call, a ball speed and player speed powerup is created.
 * When the powerups are collided with by the ball, they will be
 * destroyed and the respective body will increase in speed.
 */
void handle_powerup_spawning(state_t *state) {
  body_t *player = scene_get_body(state->scene, 2);
  body_t *ball = scene_get_body(state->scene, 3);

  if (state->powerup_spawn_delay <= 0) {
    body_t *ball_boost =
        body_init(circle_init(POWERUP_RADIUS), BALL_MASS, COLOR_INDIGO);
    body_t *player_boost =
        body_init(circle_init(POWERUP_RADIUS), BALL_MASS, COLOR_GREEN);
    body_set_centroid(ball_boost, random_powerup_loc());
    body_set_centroid(player_boost, random_powerup_loc());

    scene_add_body(state->scene, ball_boost);
    scene_add_body(state->scene, player_boost);
    create_speed_boost_collision(state->scene, BOOST_FACTOR, ball_boost, ball,
                                 ball);
    create_speed_boost_collision(state->scene, BOOST_FACTOR, player_boost, ball,
                                 player);
    state->powerup_spawn_delay =
        POWERUP_SPAWN_DELAY * (double)rand() / RAND_MAX;
  }
}

/**
 * Keypress handler to perform rotation/translation based on duration pressed.
 */
void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *player = scene_get_body(state->scene, 2);
  if (type == KEY_PRESSED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(player, (vector_t){-PLAYER_SPEED, 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(player, (vector_t){PLAYER_SPEED, 0});
      break;
    }
  }
  if (type == KEY_RELEASED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(player, VEC_ZERO);
      break;
    case RIGHT_ARROW:
      body_set_velocity(player, VEC_ZERO);
      break;
    }
  }
}

/**
 * Handles when the player hits a border wall, and
 * confines them by setting their centroid.
 */
void player_within_walls(state_t *state) {
  body_t *player = scene_get_body(state->scene, 2);
  special_state_t special = get_special_state(player);

  if (special == TOUCHING_LEFT) {
    body_set_centroid(player, LEFTMOST_PLAYER_CENTROID);
  } else if (special == TOUCHING_RIGHT) {
    body_set_centroid(player, RIGHTMOST_PLAYER_CENTROID);
  }
}

state_t *emscripten_init() {
  srand(time(NULL));

  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);
  sdl_on_key((void *)on_key);

  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;
  reset_screen(state);

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  scene_t *scene = state->scene;
  state->powerup_spawn_delay -= dt;
  scene_tick(scene, dt);
  ball_within_walls(state);
  player_within_walls(state);
  handle_powerup_spawning(state);

  sdl_render_scene(scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}