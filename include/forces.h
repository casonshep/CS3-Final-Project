#ifndef __FORCES_H__
#define __FORCES_H__

#include "scene.h"

// TODO (added for forces to work in tankz demo)
typedef struct const_body_aux {
  list_t *consts;
  list_t *bodies;
  void *extra_var;
  free_func_t freer;
} const_body_aux_t;

/**
 * A function called when a collision occurs.
 * @param body1 the first body passed to create_collision()
 * @param body2 the second body passed to create_collision()
 * @param axis a unit vector pointing from body1 towards body2
 *   that defines the direction the two bodies are colliding in
 * @param aux the auxiliary value passed to create_collision()
 */
typedef void (*collision_handler_t)(body_t *body1, body_t *body2, vector_t axis,
                                    void *aux);

// TODO (added for forces to work in tankz demo)
void const_body_aux_free(void *aux);

// TODO (added for forces to work in tankz demo)
const_body_aux_t *const_body_aux_init(list_t *consts, list_t *bodies);

/**
 * Adds a force creator to a scene that applies gravity between two bodies.
 * The force creator will be called each tick
 * to compute the Newtonian gravitational force between the bodies.
 * See
 * https://en.wikipedia.org/wiki/Newton%27s_law_of_universal_gravitation#Vector_form.
 * The force should not be applied when the bodies are very close,
 * because its magnitude blows up as the distance between the bodies goes to 0.
 *
 * @param scene the scene containing the bodies
 * @param G the gravitational proportionality constant
 * @param body1 the first body
 * @param body2 the second body
 */
void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2);

/**
 * Adds a force creator to a scene that acts like a spring between two bodies.
 * The force creator will be called each tick
 * to compute the Hooke's-Law spring force between the bodies.
 * See https://en.wikipedia.org/wiki/Hooke%27s_law.
 *
 * @param scene the scene containing the bodies
 * @param k the Hooke's constant for the spring
 * @param body1 the first body
 * @param body2 the second body
 */
void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies a drag force on a body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its velocity.
 * The force points opposite the body's velocity.
 *
 * @param scene the scene containing the bodies
 * @param gamma the proportionality constant between force and velocity
 *   (higher gamma means more drag)
 * @param body the body to slow down
 */
void create_drag(scene_t *scene, double gamma, body_t *body);

/**
 * Adds a force creator to a scene that randomly applies an impulse to
 * a body each tick.
 *
 * @param scene the scene containing the bodies
 * @param probability the probability the random impule
 * will be appplied
 * @param max_impulse max impulse value to be applied randomly
 * @param body the body to be impulsed
 */
void create_random_impulse(scene_t *scene, double probability,
                           double max_impulse, body_t *body);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies collide.
 * This generalizes create_destructive_collision() from last week,
 * allowing different things to happen on a collision.
 * The handler is passed the bodies, the collision axis, and an auxiliary value.
 * It should only be called once while the bodies are still colliding.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer);

/**
 * Adds a force creator to a scene that destroys two bodies when they collide.
 * The bodies should be destroyed by calling body_remove().
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 */
void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies impulses
 * to resolve collisions between two bodies in the scene.
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * You may remember from project01 that you should avoid applying impulses
 * multiple times while the bodies are still colliding.
 * You should also have a special case that allows either body1 or body2
 * to have mass INFINITY, as this is useful for simulating walls.
 *
 * @param scene the scene containing the bodies
 * @param elasticity the "coefficient of restitution" of the collision;
 * 0 is a perfectly inelastic collision and 1 is a perfectly elastic collision
 * @param body1 the first body
 * @param body2 the second body
 */
void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2);

/**
 * Adds a force creator to a scene that destroys only ONE body when they
 * collide. The FIRST bodies should be destroyed on collision. This should be
 * represented as an on-collision callback registered with create_collision().
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 */
void create_single_destructive_collision(scene_t *scene, body_t *body1,
                                         body_t *body2);

/**
 * Adds a force creator to a scene that destroys only ONE body when they
 * collide, and increases the speed of another body by boost_factor.
 *
 * @param scene the scene containing the bodies
 * @param boost_factor the factor at which the third body will be boosted
 * @param body1 the first body (to be removed)
 * @param body2 the second body
 * @param body3 the third body (to be boosted)
 */
void create_speed_boost_collision(scene_t *scene, double boost_factor,
                                  body_t *body1, body_t *body2, body_t *body3);

/**
 * Adds a force creator to a scene that destroys only ONE body when they
 * collide, and increments the color of the other body, until out of colors,
 * where the second body will be destroyed.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body (to be incremented on collision)
 * @param body2 the second body (to be destroyed on collision)
 * @param color_list list of colors to increment through
 * @param color_list_freer freer for the colors list when the force finishes
 */
void create_color_increment_collision(scene_t *scene, body_t *body1,
                                      body_t *body2, list_t *color_list,
                                      free_func_t color_list_freer);

#endif // #ifndef __FORCES_H__
