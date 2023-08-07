#include "color.h"
#include "forces.h"
#include "test_util.h"
#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

const rgb_color_t TEST_COLOR_BLACK = {0, 0, 0};

const double TWO_PI = 2.0 * M_PI;

double test_vec_dist(vector_t v1, vector_t v2) {
  vector_t diff = vec_subtract(v1, v2);
  diff.x = diff.x * diff.x;
  diff.y = diff.y * diff.y;
  return sqrt(diff.x + diff.y);
}

list_t *test_make_star() {
  double inner_radius = 15;
  double outer_radius = 30;
  size_t num_points = 10;
  vector_t center = VEC_ZERO;
  double vertex_angle = TWO_PI / num_points;
  list_t *vertices = list_init(num_points, free);
  assert(vertices != NULL);

  for (size_t i = 0; i < num_points; ++i) {
    // Calculate polar coordinates relative to center
    double radius = (i % 2 == 0) ? outer_radius : inner_radius;
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

// Test that objects undergoing drag force slow down at a decreasing rate
void test_drag_step() {
  const double gamma = .05;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  double mass = 10;
  vector_t initial_velocity = {50, 50};
  scene_t *scene = scene_init();
  body_t *body = body_init(test_make_star(), mass, TEST_COLOR_BLACK);
  body_set_centroid(body, VEC_ZERO);
  body_set_velocity(body, initial_velocity);
  scene_add_body(scene, body);

  create_drag(scene, gamma, body);
  double dist_from_initial_velocity = 0;
  double prev_speed = test_vec_dist(initial_velocity, VEC_ZERO);

  for (int i = 0; i < STEPS; i++) {
    scene_tick(scene, DT);
    if (vec_equal(body_get_velocity(body), VEC_ZERO)) {
      break;
    }
    double curr_dist = test_vec_dist(body_get_velocity(body), initial_velocity);
    double curr_speed = test_vec_dist(body_get_velocity(body), VEC_ZERO);
    assert(dist_from_initial_velocity <
           curr_dist);               // deceleration is decreasing
    assert(curr_speed < prev_speed); // speed is decreasing
    dist_from_initial_velocity = curr_dist;
  }
  scene_free(scene);
}

// Test that large drag coefficients slow down faster
void test_drag_coeff() {
  size_t num_bodies = 8;
  vector_t initial_velocity = {1000, 1000};
  double initial_gamma = .02;
  double mass = 10;
  scene_t *scene = scene_init();

  // create multiply fast objects with the same initial velocity
  // each with different drag coefficients
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = body_init(test_make_star(), mass, TEST_COLOR_BLACK);
    body_set_centroid(body, VEC_ZERO);
    body_set_velocity(body, initial_velocity);
    scene_add_body(scene, body);
    create_drag(scene, initial_gamma * i, body);
  }

  const double DT = 1e-6;
  const int STEPS = 1000000;
  for (int i = 0; i < STEPS; i++) {
    // check if LAST body is stopped
    body_t *last_body = scene_get_body(scene, num_bodies - 1);
    if (vec_equal(VEC_ZERO, body_get_velocity(last_body))) {
      break;
    }
    // compare bodies in pairs through list (effectively compares all)
    for (size_t idx = 0; idx < num_bodies - 1; idx++) {
      body_t *body1 = scene_get_body(scene, idx);
      body_t *body2 = scene_get_body(scene, idx + 1);
      vector_t body1_vel = body_get_velocity(body1);
      vector_t body2_vel = body_get_velocity(body2);
      double body1_speed = test_vec_dist(body1_vel, VEC_ZERO);
      double body2_speed = test_vec_dist(body2_vel, VEC_ZERO);
      assert(body1_speed >= body2_speed);
    }
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

// Test that drag doesn't change direction
void test_drag_dir() {
  size_t num_bodies = 8;
  double initial_speed = 15;
  double gamma = .1;
  double mass = 10;
  scene_t *scene = scene_init();
  vector_t dir_right = {1, 0};

  // create multiple objects same drag coefficients
  // each with different initial velocity
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = body_init(test_make_star(), mass, TEST_COLOR_BLACK);
    body_set_centroid(body, VEC_ZERO);
    vector_t vel = vec_multiply(initial_speed,
                                vec_rotate(dir_right, TWO_PI * i / num_bodies));
    body_set_velocity(body, vel);
    scene_add_body(scene, body);
    create_drag(scene, gamma, body);
  }

  const double DT = 1e-6;
  const int STEPS = 1000000;
  for (int i = 0; i < STEPS; i++) {
    for (size_t idx = 0; idx < num_bodies; idx++) {
      body_t *body = scene_get_body(scene, idx);
      vector_t vel = body_get_velocity(body);
      if (vec_equal(vel, VEC_ZERO)) {
        break;
      }
      vector_t dir = vec_multiply(1.0 / test_vec_dist(vel, VEC_ZERO), vel);
      vector_t expected_dir = vec_rotate(dir_right, TWO_PI * idx / num_bodies);
      assert(vec_within(1e-6, dir, expected_dir));
    }
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_drag_step)
  DO_TEST(test_drag_coeff)
  DO_TEST(test_drag_dir)

  puts("student_test PASS");
}