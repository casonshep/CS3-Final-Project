#include "player.h"

#include "body.h"
#include "list.h"
#include "polygon.h"
#include "vector.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

struct player {
  int player_id;
  double player_health;
  int shots_remaining;
  bool active_status;
  bool is_dead;
  body_t *body;
  void *info;
  free_func_t info_freer;
};

player_t *player_init(body_t *body) {
  return body_init_with_info(body, player_id, player_health, shots_remaining,
                             NULL);
}

player_t *player_init_with_info(body_t *body, int player_id,
                                double player_health, int shots_remaining,
                                void *info, free_func_t info_freer) {

  player_t *player = malloc(sizeof(player_t));
  player->player_id = player_id;
  player->player_health = player_health;
  player->shots_remaining = shots_remaining;
  player->is_dead = false;
  player->body = body;
  player->player_id = player_id;
  player->player_id = player_id;

  player->info = info;
  player->info_freer = info_freer;

  return player;
}

void player_free(player_t *player) {
  if (player->info_freer != NULL) {
    player->info_freer(player->info);
  }
  free(player);
}

body_t *player_get_body(player_t *player) { return player->body; }

int player_get_id(player_t *player) { return player->player_id; }

double player_get_health(player_t *player) { return player->player_health; }

int player_get_remainingshots(player_t *player) {
  return player->shots_remaining;
}

bool player_get_status(player_t *player) { return player->active_status; }

bool player_get_deadornot(player_t *player) { return player->is_dead; }

void *player_get_info(player_t *player) { return player->info; }

void player_set_remainingshots(player_t *player, int shots) {
  player->shots_remaining = shots;
}

void player_set_health(player_t *player, double health) {
  player->player_health = health;
}

void body_set_alive(player_t *player, double health) {
  player_set_health(player, health);
  player->is_dead = false;
}
