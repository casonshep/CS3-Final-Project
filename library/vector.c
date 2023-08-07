#include "vector.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
  return (vector_t){v1.x + v2.x, v1.y + v2.y};
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  return (vector_t){v1.x - v2.x, v1.y - v2.y};
}

vector_t vec_negate(vector_t v) { return vec_multiply(-1, v); }

vector_t vec_multiply(double scalar, vector_t v) {
  return (vector_t){scalar * v.x, scalar * v.y};
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
  return (vector_t){v.x * cos(angle) - v.y * sin(angle),
                    v.x * sin(angle) + v.y * cos(angle)};
}

vector_t vec_rotate_90(vector_t v, bool counterclockwise) {
  return counterclockwise ? (vector_t){-v.y, v.x} : (vector_t){v.y, -v.x};
}

double vec_dist(vector_t v1, vector_t v2) {
  return vec_norm(vec_subtract(v1, v2));
}

double vec_norm(vector_t v) { return sqrt(vec_dot(v, v)); }

vector_t vec_normalize(vector_t v) {
  if (v.x == 0 && v.y == 0) {
    return VEC_ZERO;
  }

  return vec_multiply(1.0 / vec_norm(v), v);
}

double vec_scalar_proj(vector_t axis, vector_t v, bool normalize) {
  if (axis.x == 0 && axis.y == 0) {
    return 0;
  }

  vector_t normalized_axis = normalize ? vec_normalize(axis) : axis;
  return vec_dot(v, normalized_axis);
}

vector_t vec_vec_proj(vector_t axis, vector_t v, bool normalize) {
  if (axis.x == 0 && axis.y == 0) {
    return VEC_ZERO;
  }

  if (normalize) {
    return vec_multiply(vec_dot(v, axis) / vec_dot(axis, axis), axis);
  } else {
    return vec_multiply(vec_dot(v, axis), axis);
  }
}