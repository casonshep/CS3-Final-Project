#include "collision.h"
#include "test_util.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define NUM_VERTS(VERTS) (sizeof(VERTS) / sizeof(vector_t))

const vector_t COLLIDING_PAIR1_VERTS1[] = {{1, 1}, {3, 1}, {3, 2}, {1, 2}};
const vector_t COLLIDING_PAIR1_VERTS2[] = {{2, 1}, {4, 1}, {4, 2}, {2, 2}};

const vector_t COLLIDING_PAIR2_VERTS1[] = {{1, 1}, {2, 1}, {2, 3}, {1, 3}};
const vector_t COLLIDING_PAIR2_VERTS2[] = {{1, 2}, {2, 2}, {2, 4}, {1, 4}};

const vector_t NONCOLLIDING_PAIR1_VERTS1[] = {{1, 1}, {2, 1}, {2, 2}, {1, 2}};
const vector_t NONCOLLIDING_PAIR1_VERTS2[] = {{3, 1}, {4, 1}, {4, 2}, {3, 2}};

const vector_t NONCOLLIDING_PAIR2_VERTS1[] = {{1, 1}, {2, 1}, {2, 2}, {1, 2}};
const vector_t NONCOLLIDING_PAIR2_VERTS2[] = {{1, 3}, {2, 3}, {2, 4}, {1, 4}};

list_t *shape_from_verts(vector_t verts[], size_t num_verts) {
  list_t *shape = list_init(num_verts, free);
  for (size_t i = 0; i < num_verts; ++i) {
    vector_t *vert = malloc(sizeof(vector_t));
    *vert = verts[i];
    list_add(shape, vert);
  }
  return shape;
}

void test_pair(bool colliding, vector_t verts1[], size_t num_verts1,
               vector_t verts2[], size_t num_verts2) {
  list_t *shape1 = shape_from_verts(verts1, num_verts1);
  list_t *shape2 = shape_from_verts(verts2, num_verts2);
  assert(find_collision(shape1, shape2).collided == colliding);
  list_free(shape1);
  list_free(shape2);
}

void test_colliding() {
  test_pair(true, (vector_t *)COLLIDING_PAIR1_VERTS1,
            NUM_VERTS(COLLIDING_PAIR1_VERTS1),
            (vector_t *)COLLIDING_PAIR1_VERTS2,
            NUM_VERTS(COLLIDING_PAIR1_VERTS2));
  test_pair(true, (vector_t *)COLLIDING_PAIR2_VERTS1,
            NUM_VERTS(COLLIDING_PAIR2_VERTS1),
            (vector_t *)COLLIDING_PAIR2_VERTS2,
            NUM_VERTS(COLLIDING_PAIR2_VERTS2));
}

void test_noncolliding() {
  test_pair(false, (vector_t *)NONCOLLIDING_PAIR1_VERTS1,
            NUM_VERTS(NONCOLLIDING_PAIR1_VERTS1),
            (vector_t *)NONCOLLIDING_PAIR1_VERTS2,
            NUM_VERTS(NONCOLLIDING_PAIR1_VERTS2));
  test_pair(false, (vector_t *)NONCOLLIDING_PAIR2_VERTS1,
            NUM_VERTS(NONCOLLIDING_PAIR2_VERTS1),
            (vector_t *)NONCOLLIDING_PAIR2_VERTS2,
            NUM_VERTS(NONCOLLIDING_PAIR2_VERTS2));
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_colliding)
  DO_TEST(test_noncolliding)

  puts("collision_test PASS");
}
