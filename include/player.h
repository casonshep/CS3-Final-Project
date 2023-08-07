#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "body.h"
#include "color.h"
#include "list.h"
#include "vector.h"
#include <stdbool.h>

/**
 * A player constrained to the game.
 * Holds data of body and in-game stats
 */
typedef struct player player_t;

/**
 * Initializes a player without any info.
 * Acts like player_init_with_info() where info and info_freer are NULL.
 */
player_t *player_init(body_t *body);

player_t *player_init_with_info(body_t *body, int player_id,
                                double player_health, int shots_remaining,
                                void *info, free_func_t info_freer);

void player_free(player_t *player);

body_t *player_get_body(player_t *player);

int player_get_id(player_t *player);

double player_get_health(player_t *player);

int player_get_remainingshots(player_t *player);

bool player_get_status(player_t *player);

bool player_get_deadornot(player_t *player);

void *player_get_info(player_t *player);

void player_set_remainingshots(player_t *player, int shots);

void player_set_health(player_t *player, double health);

void body_set_alive(player_t *player, double health);

#endif // #ifndef __PLAYER_H__
