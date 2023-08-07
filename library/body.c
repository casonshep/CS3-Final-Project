#include "body.h"

#include "list.h"
#include "polygon.h"
#include "vector.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

struct body {
  list_t *shape;
  double mass;
  double rotation;
  rgb_color_t color;
  vector_t centroid;
  vector_t velocity;
  vector_t force;
  vector_t impulse;
  void *info;
  free_func_t info_freer;
  bool is_removed;
};

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  return body_init_with_info(shape, mass, color, NULL, NULL);
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
  vector_t centroid = polygon_centroid(shape);
  double rotation = 0;

  body_t *body = malloc(sizeof(body_t));
  body->shape = shape;
  body->mass = mass;
  body->color = color;
  body->centroid = centroid;
  body->velocity = VEC_ZERO;
  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
  body->rotation = rotation;
  body->is_removed = false;

  body->info = info;
  body->info_freer = info_freer;

  return body;
}

void body_free(body_t *body) {
  list_free(body->shape);
  if (body->info_freer != NULL) {
    body->info_freer(body->info);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) {
  list_t *shape = body->shape;
  size_t size = list_size(shape);
  list_t *shape_copy = list_init(size, free);
  for (size_t i = 0; i < size; ++i) {
    vector_t *vtx = list_get(shape, i);
    vector_t *vtx_cpy = malloc(sizeof(vector_t));
    vtx_cpy->x = vtx->x;
    vtx_cpy->y = vtx->y;
    list_add(shape_copy, vtx_cpy);
  }
  return shape_copy;
}

vector_t body_get_centroid(body_t *body) { return body->centroid; }

vector_t body_get_velocity(body_t *body) { return body->velocity; }

double body_get_rotation(body_t *body) { return body->rotation; }

double body_get_mass(body_t *body) { return body->mass; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void *body_get_info(body_t *body) { return body->info; }

void body_set_centroid(body_t *body, vector_t x) {
  vector_t displacement = vec_subtract(x, body->centroid);

  polygon_translate(body->shape, displacement);
  body->centroid = x;
}

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_rotation(body_t *body, double angle) {
  double relative_angle = angle - body->rotation;

  polygon_rotate(body->shape, relative_angle, body->centroid);
  body->rotation = angle;
}

void body_set_color(body_t *body, rgb_color_t color) { body->color = color; }

void body_add_force(body_t *body, vector_t force) {
  body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->impulse = vec_add(body->impulse, impulse);
}

void body_tick(body_t *body, double dt) {
  vector_t total_impulse =
      vec_add(body->impulse, vec_multiply(dt, body->force));
  vector_t dv = vec_multiply(1.0 / body_get_mass(body), total_impulse);

  vector_t old_velocity = body_get_velocity(body);
  vector_t new_velocity = vec_add(old_velocity, dv);
  vector_t avg_velocity =
      vec_multiply(0.5, vec_add(old_velocity, new_velocity));

  vector_t displacement = vec_multiply(dt, avg_velocity);
  vector_t new_centroid = vec_add(body->centroid, displacement);

  body_set_centroid(body, new_centroid);
  body_set_velocity(body, new_velocity);
  body->impulse = VEC_ZERO;
  body->force = VEC_ZERO;
}

void body_remove(body_t *body) { body->is_removed = true; }

bool body_is_removed(body_t *body) { return body->is_removed; }
