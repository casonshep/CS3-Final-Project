#include "scene.h"
#include "list.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

const size_t INIT_BODY_CAPACITY = 8;
const size_t INIT_FORCE_CAPACITY = 8;

struct scene {
  list_t *bodies;
  list_t *forces;
};

typedef struct force {
  force_creator_t forcer;
  void *aux;
  list_t *bodies;
  free_func_t freer;
  bool mark_removal;
} force_t;

void force_free(force_t *force) {
  if (force->freer != NULL) {
    force->freer(force->aux);
  }
  // Remove the bodies from the list of bodies so as to not free them (we only
  // want to free the list)
  while (list_size(force->bodies) != 0) {
    list_remove(force->bodies, 0);
  }
  list_free(force->bodies);
  free(force);
}

// used to mark forces for removal in scene_tick
void force_remove(force_t *force) { force->mark_removal = true; }

// returns true if a force is marked for removal
bool force_is_removed(force_t *force) { return force->mark_removal; }

scene_t *scene_init() {
  list_t *bodies = list_init(INIT_BODY_CAPACITY, (free_func_t)body_free);
  list_t *forces = list_init(INIT_FORCE_CAPACITY, (free_func_t)force_free);

  scene_t *scene = malloc(sizeof(scene_t));
  scene->bodies = bodies;
  scene->forces = forces;
  return scene;
}

void scene_free(scene_t *scene) {
  list_free(scene->forces);
  list_free(scene->bodies);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  assert(index < list_size(scene->bodies));

  return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_force(scene_t *scene, size_t index) {
  assert(index < list_size(scene->forces));

  force_t *force = list_remove(scene->forces, index);
  force_free(force);
}

void scene_remove_body(scene_t *scene, size_t index) {
  assert(index < list_size(scene->bodies));

  body_t *body = list_get(scene->bodies, index);
  body_remove(body);
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
  scene_add_bodies_force_creator(scene, forcer, aux, NULL, freer);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  force_t *force = malloc(sizeof(force_t));
  force->aux = aux;

  // initialized empty list if not passed in (allows use of deprecated
  // scene_add_force_creator)
  force->bodies = (bodies == NULL) ? list_init(0, NULL) : bodies;

  force->mark_removal = false;
  force->freer = freer;
  force->forcer = forcer;
  list_add(scene->forces, force);
}

void scene_tick(scene_t *scene, double dt) {
  size_t num_forces = list_size(scene->forces);

  for (size_t i = 0; i < num_forces; i++) {
    force_t *force = (force_t *)list_get(scene->forces, i);

    // applies each force
    force->forcer(force->aux);
  }

  for (size_t i = 0; i < num_forces; i++) {
    force_t *force = (force_t *)list_get(scene->forces, i);

    // checks list of bodies in each force. if any are marked for removal,
    // the force is also marked for removal
    for (size_t j = 0; j < list_size(force->bodies); j++) {
      body_t *curr_body = list_get(force->bodies, j);
      if (body_is_removed(curr_body) == true) {
        force_remove(force);
      }
    }
  }

  // loops through bodies, if marked for removal, removes it
  // otherwise, body_tick is applied
  size_t i = 0;
  while (i < list_size(scene->bodies)) {
    body_t *body = list_get(scene->bodies, i);
    if (body_is_removed(body) == true) {
      body_t *body_remove = list_remove(scene->bodies, i);
      body_free(body_remove);
    } else {
      body_tick(body, dt);
      i++;
    }
  }

  // loops through forces, if marked for removal, removes it
  size_t j = 0;
  while (j < list_size(scene->forces)) {
    force_t *force = list_get(scene->forces, j);
    if (force_is_removed(force) == true) {
      scene_remove_force(scene, j);
    } else {
      j++;
    }
  }
}