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

// Border constants
const size_t BORDER_LENGTH = 150;
const vector_t BORDER1_CENTER = {.x = 75, .y = 250};
const vector_t BORDER2_CENTER = {.x = 925, .y = 250};
const rgb_color_t BORDER_COLOR = COLOR_BLACK;

// Player constants
const size_t PLAYER_NUM_PTS = 40;
const double PLAYER_X_RADIUS = 30;
const double PLAYER_Y_RADIUS = 10;
const rgb_color_t PLAYER_COLOR = COLOR_LIME;
const double PLAYER_MASS = 1;
const vector_t PLAYER_INIT_POSITION = {.x = 500, .y = 15};
const vector_t RESET_PLAYER_RIGHT = {819, 15};
const vector_t RESET_PLAYER_LEFT = {181, 15};

const double PLAYER_SPEED = 300;

// Invader constants
const size_t INVADER_NUM_CIRC_PTS = 40;
const double INVADER_RADIUS = 30;
const rgb_color_t INVADER_COLOR = (rgb_color_t){0.52, 0.52, 0.52};
const double INVADER_MASS = 1;
const size_t INIT_NUM_INVADERS = 24;
const vector_t FIRST_INVADER_CENTER = {.x = 200, .y = 450};
const vector_t INVADER_SHIFT = {.x = 70, .y = 0};
const vector_t INVADER_DOWN_SHIFT = {.x = 560, .y = 50};
const double INVADER_DOWN_SHIFT_SCALE = 3;
const double INVADER_TAPER_ANGLE = 45.0 / 72.0 * TWO_PI;
const double INVADER_SPEED = 50;

// Shot constants
const size_t SHOT_LENGTH = 5;
const size_t SHOT_HEIGHT = 10;
const vector_t SHOT_VELOCITY = {.x = 0, .y = 600};
const size_t INVADER_SHOT_DELAY = 5;

/**
 * Generates Player vertices list given max radius of Player, number of vertices
 * of Player, and center coordinates (vector_t) of Player.
 * Returns list_t of vertices of Player.
 */
list_t *make_player(double x_radius, double y_radius, size_t num_pts,
                    vector_t center) {
  // Initialize angles and Player
  double curr_angle = 0;
  double vert_angle = TWO_PI / num_pts;
  list_t *vertices = list_init(num_pts, free);
  assert(vertices != NULL);

  // Add vertices to Player
  for (size_t i = 0; i < num_pts; ++i) {
    // Calculate cartesian coordinates
    double x = x_radius * cos(curr_angle) + center.x;
    double y = y_radius * sin(curr_angle) + center.y;

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
 * Generates Invader vertices list given radius of invader, number of vertices
 * of Invader, and center coordinates (vector_t) of Invader.
 * Returns list_t of vertices of Invader.
 */
list_t *make_invader(double radius, size_t num_circ_pts, vector_t center) {
  // Initialize angles and Invader polygon
  size_t num_pts_skipped = INVADER_TAPER_ANGLE / TWO_PI * num_circ_pts;
  double vert_angle = TWO_PI / num_circ_pts;
  double curr_angle = (INVADER_TAPER_ANGLE / 2) + (3 * TWO_PI / 4);
  size_t num_pts_drawn =
      num_circ_pts - num_pts_skipped + 1; // doesn't include tapered portion
  list_t *vertices = list_init(num_pts_drawn, free);
  assert(vertices != NULL);

  // Add center vertex
  vector_t *malloced_center = malloc(sizeof(vector_t));
  malloced_center->x = center.x;
  malloced_center->y = center.y;
  list_add(vertices, malloced_center);

  for (size_t i = 0; i < num_pts_drawn; ++i) {
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

struct state {
  scene_t *scene;
  double time_until_next_shot;
  size_t invaders_remaining;
  bool ready_to_shoot;
};

typedef enum body_type { WALL, PLAYER, INVADER, BULLET } body_type_t;

/**
 * Creates body info based on given body type to be passed to
 * body_init_with_info with free as its freer.
 */
body_type_t *create_info(body_type_t body_type) {
  body_type_t *info = malloc(sizeof(body_type_t));
  *info = body_type;
  return info;
}

/**
 * Returns the body associated with the player.
 * Returns NULL if the player does not exist.
 */
body_t *get_player(state_t *state) {
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = *(body_type_t *)body_get_info(body);
    if (type == PLAYER) {
      return body;
    }
  }
  return NULL;
}

/**
 * Makes the given body shoot (can be the player or an invader).
 */
void shape_shoot(body_t *shooting_body, state_t *state) {
  body_type_t shooting_body_type = *(body_type_t *)body_get_info(shooting_body);
  vector_t center = body_get_centroid(shooting_body);
  rgb_color_t color = body_get_color(shooting_body);

  list_t *vertices = make_rectangle(SHOT_LENGTH, SHOT_HEIGHT, center);
  body_t *shot = body_init_with_info(vertices, PLAYER_MASS, color,
                                     create_info(BULLET), free);

  if (shooting_body_type == PLAYER) { // Player is shooting
    body_set_velocity(shot, SHOT_VELOCITY);
    // Add destructive collisions between shot and all existing invaders
    for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
      body_t *body = scene_get_body(state->scene, i);
      body_type_t type = *(body_type_t *)body_get_info(body);
      if (type != INVADER) {
        continue;
      }
      create_destructive_collision(state->scene, shot, body);
    }
  } else { // Invader is shooting
    body_set_velocity(shot, vec_negate(SHOT_VELOCITY));
    // Add destructive collision between shot and player
    create_destructive_collision(state->scene, shot, get_player(state));
  }

  scene_add_body(state->scene, shot);
}

/**
 * Generates shots from remaining invaders in the scene in a set time interval,
 * INVADER_SHOT_DELAY (seconds)
 */
void handle_invader_shooting(state_t *state) {
  if (state->time_until_next_shot > 0) {
    return;
  }
  size_t remaining_invaders = state->invaders_remaining;
  if (remaining_invaders == 0) {
    return;
  }
  size_t random_invader_idx =
      (size_t)((double)rand() / RAND_MAX * remaining_invaders);
  size_t invader_count = 0;
  body_t *random_invader = NULL;
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = *(body_type_t *)body_get_info(body);
    if (type != INVADER) {
      continue;
    }
    if (invader_count == random_invader_idx) {
      random_invader = body;
      break;
    }
    ++invader_count;
  }
  assert(random_invader != NULL);

  shape_shoot(random_invader, state);
  state->time_until_next_shot = INVADER_SHOT_DELAY;
}
typedef enum special_state {
  NO_SPECIAL_STATE,
  OFF_SCREEN,
  TOUCHING_RIGHT,
  TOUCHING_LEFT
} special_state_t;

/**
 * Returns OFF_SCREEN if the shape is completely off its designated space,
 * TOUCHING_RIGHT if the shape is touching the right border, TOUCHING_LEFT if
 * the shape is touching the left border, and NO_SPECIAL_STATE otherwise;
 */
special_state_t get_special_state(body_t *body) {
  body_type_t type = *(body_type_t *)body_get_info(body);
  // Ensure that invaders are counted as off screen when on the level of the
  // player
  double bottom_boundary = (type == PLAYER) ? 0 : 2.0 * INVADER_RADIUS;
  vector_t center = body_get_centroid(body);

  // Iterate only through outer vertices
  if (center.y + INVADER_RADIUS < bottom_boundary ||
      center.y - INVADER_RADIUS > WINDOW.y) {
    return OFF_SCREEN;
  } else if (center.x + INVADER_RADIUS >= WINDOW.x - BORDER_LENGTH) {
    return TOUCHING_RIGHT;
  } else if (center.x - INVADER_RADIUS <= BORDER_LENGTH) {
    return TOUCHING_LEFT;
  }
  return NO_SPECIAL_STATE;
}

/**
 * Stops the player if they hit a wall.
 */
void handle_player_wall_hitting(state_t *state) {
  body_t *player = get_player(state);
  special_state_t player_special_state = get_special_state(player);

  switch (player_special_state) {
  case TOUCHING_LEFT:
  case TOUCHING_RIGHT:
    // Stop the player from moving further and reset centroid
    body_set_centroid(player, (player_special_state == TOUCHING_LEFT)
                                  ? RESET_PLAYER_LEFT
                                  : RESET_PLAYER_RIGHT);
    double x_velocity = body_get_velocity(player).x;
    if ((player_special_state == TOUCHING_LEFT) ? x_velocity < 0
                                                : x_velocity > 0) {
      body_set_velocity(player, VEC_ZERO);
    }
    break;
  case OFF_SCREEN:
    break;
  case NO_SPECIAL_STATE:
    break;
  }
}

/**
 * Sends invaders to next row when they hit a wall. Returns true if any invaders
 * are below the screen.
 */
bool handle_invader_wall_hitting(state_t *state) {
  bool any_off_screen = false;
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = *(body_type_t *)body_get_info(body);
    if (type != INVADER) {
      continue;
    }
    special_state_t body_special_state = get_special_state(body);

    vector_t curr_centroid = body_get_centroid(body);
    vector_t curr_velocity = body_get_velocity(body);
    vector_t new_velocity = {.x = -1 * curr_velocity.x, .y = curr_velocity.y};
    double x = 0;
    double y = 0;
    switch (body_special_state) {
    case OFF_SCREEN:
      // Remove the body
      scene_remove_body(state->scene, i);
      any_off_screen = true;
      break;
    case TOUCHING_LEFT:
    case TOUCHING_RIGHT:
      // Shift the invader down and redirect velocity
      x = (body_special_state == TOUCHING_LEFT)
              ? BORDER_LENGTH + INVADER_RADIUS
              : WINDOW.x - BORDER_LENGTH - INVADER_RADIUS;
      y = curr_centroid.y - (INVADER_DOWN_SHIFT.y * INVADER_DOWN_SHIFT_SCALE);
      vector_t new_centroid = {.x = x, .y = y};
      body_set_centroid(body, new_centroid);
      body_set_velocity(body, new_velocity);

      // Check off-screen condition on new position
      if (get_special_state(body) == OFF_SCREEN) {
        // Remove the body
        scene_remove_body(state->scene, i);
        any_off_screen = true;
      }
      break;
    case NO_SPECIAL_STATE:
      break;
    }
  }
  return any_off_screen;
}

/**
 * Returns true iff the game has ended by either player destruction of
 * destruction of all invaders.
 */
bool handle_destruction_game_end(state_t *state) {
  if (state->invaders_remaining == 0) {
    // Win
    return true;
  } else {
    // Try to find player
    bool player_exists = false;
    for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
      if (*(body_type_t *)body_get_info(scene_get_body(state->scene, i)) ==
          PLAYER) {
        player_exists = true;
        break;
      }
    }

    if (!player_exists) {
      // Loss
      return true;
    }
  }
  return false;
}

/**
 * Keypress handler to perform rotation/translation based on duration pressed.
 */
void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *player = get_player(state);

  if (type == KEY_PRESSED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(player, (vector_t){-PLAYER_SPEED, 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(player, (vector_t){PLAYER_SPEED, 0});
      break;
    case SPACE:
      if (state->ready_to_shoot) {
        shape_shoot(
            player,
            state); // passing in index of Player in scene's list of bodies
      }
      state->ready_to_shoot = false;
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
    case SPACE:
      state->ready_to_shoot = true;
      break;
    }
  }
}

state_t *emscripten_init() {
  // Seed the rng
  srand(time(NULL));

  // Initialize state
  state_t *state = malloc(sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;
  state->time_until_next_shot = 0;
  state->invaders_remaining = INIT_NUM_INVADERS;

  // Initialize window and renderer
  vector_t min = {.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);
  sdl_on_key((void *)on_key);

  // Create Border, two black rectangles
  list_t *pts1 = make_rectangle(BORDER_LENGTH, WINDOW.y, BORDER1_CENTER);
  body_t *border1 = body_init_with_info(pts1, PLAYER_MASS, BORDER_COLOR,
                                        create_info(WALL), free);
  scene_add_body(state->scene, border1);

  list_t *pts2 = make_rectangle(BORDER_LENGTH, WINDOW.y, BORDER2_CENTER);
  body_t *border2 = body_init_with_info(pts2, PLAYER_MASS, BORDER_COLOR,
                                        create_info(WALL), free);
  scene_add_body(state->scene, border2);

  // Create Player and add to the scene
  list_t *vertices = make_player(PLAYER_X_RADIUS, PLAYER_Y_RADIUS,
                                 PLAYER_NUM_PTS, PLAYER_INIT_POSITION);
  body_t *player = body_init_with_info(vertices, PLAYER_MASS, PLAYER_COLOR,
                                       create_info(PLAYER), free);
  scene_add_body(state->scene, player);

  // Create Invaders, set intial velocity, and add to the scene
  vector_t curr_center = FIRST_INVADER_CENTER;
  for (size_t i = 1; i < INIT_NUM_INVADERS + 1; ++i) {
    list_t *vertices =
        make_invader(INVADER_RADIUS, PLAYER_NUM_PTS, curr_center);
    body_t *invader = body_init_with_info(vertices, PLAYER_MASS, INVADER_COLOR,
                                          create_info(INVADER), free);
    body_set_velocity(invader, (vector_t){INVADER_SPEED, 0});
    scene_add_body(state->scene, invader);

    curr_center = vec_add(curr_center, INVADER_SHIFT);

    if (i % 8 == 0) {
      curr_center = vec_subtract(curr_center, INVADER_DOWN_SHIFT);
    }
  }

  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  scene_t *scene = state->scene;

  scene_tick(scene, dt);
  state->invaders_remaining = 0;
  for (size_t i = 0; i < scene_bodies(state->scene); ++i) {
    if (*(body_type_t *)body_get_info(scene_get_body(state->scene, i)) ==
        INVADER) {
      ++state->invaders_remaining;
    }
  }

  if (handle_destruction_game_end(state)) {
    sdl_quit();
    return;
  }
  if (handle_invader_wall_hitting(state)) {
    sdl_quit();
    return;
  }
  handle_player_wall_hitting(state);
  handle_invader_shooting(state);

  state->time_until_next_shot -= dt;

  sdl_render_scene(scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}