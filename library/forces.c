#include "forces.h"

#include "collision.h"
#include "list.h"
#include "scene.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

const double GRAVITY_MIN_DIST = 5;

// TODO
// typedef struct const_body_aux {
//   list_t *consts;
//   list_t *bodies;
//   void *extra_var;
//   free_func_t freer;
// } const_body_aux_t;

typedef struct force_aux {
  collision_handler_t handler;
  void *const_body_aux;
  free_func_t freer;
  bool already_colliding;
} force_aux_t;

force_aux_t *force_aux_init(collision_handler_t handler, void *aux,
                            free_func_t freer) {
  force_aux_t *force_aux = malloc(sizeof(force_aux_t));
  // used to handle collision_handler_t collisions
  force_aux->handler = handler;
  force_aux->const_body_aux = aux;
  force_aux->freer = freer;
  force_aux->already_colliding = false;
  return force_aux;
}

const_body_aux_t *const_body_aux_init(list_t *consts, list_t *bodies) {
  const_body_aux_t *aux = malloc(sizeof(const_body_aux_t));
  aux->consts = consts;
  aux->bodies = bodies;
  return aux;
}

void force_aux_free(void *aux) {
  force_aux_t *tpd_aux = aux;
  if (tpd_aux->const_body_aux != NULL) {
    tpd_aux->freer(tpd_aux->const_body_aux);
  }
  free(tpd_aux);
}

void const_body_aux_free(void *aux) {
  const_body_aux_t *tpd_aux = aux;
  // Remove the bodies from the list of bodies so as to not free them (we only
  // want to free the list)
  while (list_size(tpd_aux->bodies) != 0) {
    list_remove(tpd_aux->bodies, 0);
  }
  list_free(tpd_aux->bodies);
  list_free(tpd_aux->consts);
  free(tpd_aux);
}

void apply_newtonian(void *aux) {
  const_body_aux_t *tpd_aux = aux;
  body_t *body1 = list_get(tpd_aux->bodies, 0);
  body_t *body2 = list_get(tpd_aux->bodies, 1);
  double big_g = *(double *)list_get(tpd_aux->consts, 0);
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  double dist = vec_dist(centroid1, centroid2);
  if (dist < GRAVITY_MIN_DIST) {
    return;
  }
  vector_t dir_1_to_2 =
      vec_multiply(1.0 / dist, vec_subtract(centroid2, centroid1));
  vector_t dir_2_to_1 = vec_negate(dir_1_to_2);
  double force_mag =
      big_g * body_get_mass(body1) * body_get_mass(body2) / (dist * dist);
  body_add_force(body1, vec_multiply(force_mag, dir_1_to_2));
  body_add_force(body2, vec_multiply(force_mag, dir_2_to_1));
}

void apply_spring(void *aux) {
  const_body_aux_t *tpd_aux = aux;
  body_t *body1 = list_get(tpd_aux->bodies, 0);
  body_t *body2 = list_get(tpd_aux->bodies, 1);
  double k = *(double *)list_get(tpd_aux->consts, 0);
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  vector_t disp_1_to_2 = vec_subtract(centroid2, centroid1);
  vector_t disp_2_to_1 = vec_negate(disp_1_to_2);
  body_add_force(body1, vec_multiply(k, disp_1_to_2));
  body_add_force(body2, vec_multiply(k, disp_2_to_1));
}

void apply_drag(void *aux) {
  const_body_aux_t *tpd_aux = aux;
  body_t *body = list_get(tpd_aux->bodies, 0);
  double gamma = *(double *)list_get(tpd_aux->consts, 0);
  vector_t force = vec_multiply(-gamma, body_get_velocity(body));
  body_add_force(body, force);
}

void apply_random_impulse(void *aux) {
  const_body_aux_t *tpd_aux = aux;
  body_t *body = list_get(tpd_aux->bodies, 0);
  double probability = *(double *)list_get(tpd_aux->consts, 0);
  double max_impulse = *(double *)list_get(tpd_aux->consts, 1);

  double random_impulse_x =
      ((double)(rand() - rand()) / RAND_MAX * max_impulse);
  double random_impulse_y =
      ((double)(rand() - rand()) / RAND_MAX * max_impulse);

  double random_prob = ((double)rand() / RAND_MAX);

  if (random_prob <= probability) {
    body_add_impulse(body,
                     (vector_t){.x = random_impulse_x, .y = random_impulse_y});
  }
}

void apply_destructive_collision(body_t *body1, body_t *body2, vector_t axis,
                                 void *aux) {
  body_remove(body1);
  body_remove(body2);
}

void apply_single_destructive_collision(body_t *body1, body_t *body2,
                                        vector_t axis, void *aux) {
  body_remove(body1);
}

void apply_physics_collision(body_t *body1, body_t *body2, vector_t axis,
                             void *aux) {
  const_body_aux_t *tpd_aux = aux;
  double *elasticity = list_get(tpd_aux->consts, 0);
  if (body_get_mass(body1) == INFINITY) {
    double m = body_get_mass(body2);
    vector_t body2_speed = body_get_velocity(body2);
    double impulse = vec_dot(axis, body2_speed);
    impulse = impulse * m * (1 + *elasticity);
    body_add_impulse(body2, vec_multiply(impulse, axis));
  } else if (body_get_mass(body2) == INFINITY) {
    double m = body_get_mass(body1);
    vector_t body1_speed = body_get_velocity(body1);
    double impulse = vec_dot(axis, body1_speed);
    impulse = impulse * m * (1 + *elasticity);
    body_add_impulse(body1, vec_multiply(-1 * impulse, axis));
  } else {
    double m_1 = body_get_mass(body1);
    double m_2 = body_get_mass(body2);
    double u1 = vec_dot(body_get_velocity(body1), axis);
    double u2 = vec_dot(body_get_velocity(body2), axis);
    double reduced_mass = (m_1 * m_2) / (m_1 + m_2);
    double impulse = reduced_mass * (1 + *elasticity) * (u2 - u1);
    vector_t impulse_vec = vec_multiply(impulse, axis);
    body_add_impulse(body1, impulse_vec);
    body_add_impulse(body2, vec_multiply(-1, impulse_vec));
  }
}

void apply_speed_boost_collision(body_t *body1, body_t *body2, vector_t axis,
                                 void *aux) {
  // body 1 will get destroyed upon collision with body 2
  body_remove(body1);
  // body 3 will get a speed increase
  const_body_aux_t *const_body_aux = aux;
  double *boost_factor = list_get(const_body_aux->consts, 0);
  body_t *body3 = list_get(const_body_aux->bodies, 2);

  vector_t curr_vel = body_get_velocity(body3);
  vector_t new_vel = vec_multiply(*boost_factor, curr_vel);
  body_set_velocity(body3, new_vel);
}

void apply_color_increment_collision(body_t *body1, body_t *body2,
                                     vector_t axis, void *aux) {
  // body 2 will get destroyed upon collision with body 2
  body_remove(body1);

  // body 1 will get it's color incremented
  const_body_aux_t *const_body_aux = aux;
  list_t *color_list = (list_t *)const_body_aux->extra_var;
  rgb_color_t curr_color = body_get_color(body2);
  size_t counter = 0;
  for (size_t i = 0; i < list_size(color_list); i++) {
    rgb_color_t curr = *(rgb_color_t *)list_get(color_list, i);
    if ((curr_color.b == curr.b) && (curr_color.g == curr.g) &&
        (curr_color.r == curr.r)) {
      if (i < list_size(color_list) - 1) {
        rgb_color_t next_color = *(rgb_color_t *)list_get(color_list, i + 1);
        body_set_color(body2, next_color);
      } else {
        body_remove(body2);
      }
      counter++;
    }
  }
  if (counter == 0) {
    body_remove(body2);
  }
  const_body_aux->freer(color_list);
}

void collision(void *aux) {
  force_aux_t *force_aux = aux;
  const_body_aux_t *tpd_aux = force_aux->const_body_aux;
  body_t *body1 = list_get(tpd_aux->bodies, 0);
  body_t *body2 = list_get(tpd_aux->bodies, 1);
  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  collision_info_t bodies_are_colliding = find_collision(shape1, shape2);
  list_free(shape1);
  list_free(shape2);
  if (bodies_are_colliding.collided) {
    if (force_aux->already_colliding == false) {
      force_aux->handler(body1, body2, bodies_are_colliding.axis,
                         force_aux->const_body_aux);
    }
    force_aux->already_colliding = true;
  } else {
    force_aux->already_colliding = false;
  }
}

void create_newtonian_gravity(scene_t *scene, double big_g, body_t *body1,
                              body_t *body2) {
  // build force aux
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);
  list_t *consts = list_init(1, free);
  double *big_g_ptr = malloc(sizeof(double));
  *big_g_ptr = big_g;
  list_add(consts, big_g_ptr);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  // add force creator to the scene
  list_t *scene_bodies = list_init(2, (free_func_t)body_free);
  list_add(scene_bodies, body1);
  list_add(scene_bodies, body2);
  scene_add_bodies_force_creator(scene, apply_newtonian, aux, scene_bodies,
                                 const_body_aux_free);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
  // build force_aux_t
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);
  list_t *consts = list_init(1, free);
  double *k_ptr = malloc(sizeof(double));
  *k_ptr = k;
  list_add(consts, k_ptr);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  // add force creator to the scene
  list_t *scene_bodies = list_init(2, (free_func_t)body_free);
  list_add(scene_bodies, body1);
  list_add(scene_bodies, body2);
  scene_add_bodies_force_creator(scene, apply_spring, aux, scene_bodies,
                                 const_body_aux_free);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
  // build force_aux_t
  list_t *aux_bodies = list_init(1, (free_func_t)body_free);
  list_add(aux_bodies, body);
  list_t *consts = list_init(1, free);
  double *gamma_ptr = malloc(sizeof(double));
  *gamma_ptr = gamma;
  list_add(consts, gamma_ptr);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  // add force creator to the scene
  list_t *scene_bodies = list_init(1, (free_func_t)body_free);
  list_add(scene_bodies, body);
  scene_add_bodies_force_creator(scene, apply_drag, aux, scene_bodies,
                                 const_body_aux_free);
}

void create_random_impulse(scene_t *scene, double probability,
                           double max_impulse, body_t *body) {
  // build force_aux_t
  list_t *aux_bodies = list_init(1, (free_func_t)body_free);
  list_add(aux_bodies, body);
  list_t *consts = list_init(2, free);
  double *probability_ptr = malloc(sizeof(double));
  *probability_ptr = probability;
  list_add(consts, probability_ptr);
  double *max_impulse_ptr = malloc(sizeof(double));
  *max_impulse_ptr = max_impulse;
  list_add(consts, max_impulse_ptr);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  // add force creator to the scene
  list_t *scene_bodies = list_init(1, (free_func_t)body_free);
  list_add(scene_bodies, body);
  scene_add_bodies_force_creator(scene, apply_random_impulse, aux, scene_bodies,
                                 const_body_aux_free);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);

  force_aux_t *force_aux = force_aux_init(handler, aux, freer);

  // do aux_bodies and force_aux->bodies need to be diff lists??????
  scene_add_bodies_force_creator(scene, (force_creator_t)collision, force_aux,
                                 aux_bodies, force_aux_free);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  // build force aux
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);
  const_body_aux_t *aux = const_body_aux_init(list_init(0, NULL), aux_bodies);

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_destructive_collision, aux,
                   const_body_aux_free);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {

  // build force aux
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);

  list_t *consts = list_init(1, free);
  double *elasticity_ptr = malloc(sizeof(double));
  *elasticity_ptr = elasticity;
  list_add(consts, elasticity_ptr);

  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  // add force creator to the scene
  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_physics_collision, aux,
                   const_body_aux_free);
}

void create_single_destructive_collision(scene_t *scene, body_t *body1,
                                         body_t *body2) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);
  const_body_aux_t *aux = const_body_aux_init(list_init(0, NULL), aux_bodies);

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_single_destructive_collision, aux,
                   const_body_aux_free);
}

void create_speed_boost_collision(scene_t *scene, double boost_factor,
                                  body_t *body1, body_t *body2, body_t *body3) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);
  list_add(aux_bodies, body3);

  list_t *consts = list_init(1, free);
  double *factor_ptr = malloc(sizeof(double));
  *factor_ptr = boost_factor;
  list_add(consts, factor_ptr);

  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_speed_boost_collision, aux,
                   const_body_aux_free);
}

void create_color_increment_collision(scene_t *scene, body_t *body1,
                                      body_t *body2, list_t *color_list,
                                      free_func_t color_list_freer) {
  list_t *aux_bodies = list_init(2, (free_func_t)body_free);
  list_add(aux_bodies, body1);
  list_add(aux_bodies, body2);

  list_t *consts = list_init(1, free);
  const_body_aux_t *aux = const_body_aux_init(consts, aux_bodies);
  aux->extra_var = (void *)color_list;
  aux->freer = color_list_freer;

  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_color_increment_collision, aux,
                   const_body_aux_free);
}
