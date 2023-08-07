#include "polygon.h"

#include "list.h"
#include "vector.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

double polygon_area(list_t *polygon) {
  double det_sum = 0.0;
  size_t size = list_size(polygon);

  // Add determinants in the shoelace formula to det_sum
  for (size_t i = 0; i < size; ++i) {
    size_t j = (i + 1) % size;
    det_sum += (((vector_t *)list_get(polygon, i))->x -
                ((vector_t *)list_get(polygon, j))->x) *
               (((vector_t *)list_get(polygon, i))->y +
                ((vector_t *)list_get(polygon, j))->y);
  }

  return det_sum * 0.5;
}

vector_t polygon_centroid(list_t *polygon) {
  double area = polygon_area(polygon);
  double centroid_x = 0.0;
  double centroid_y = 0.0;
  size_t size = list_size(polygon);

  // Compute the x and y coordinates of the centroid (delaying division by 6 *
  // area)
  for (size_t i = 0; i < size; ++i) {
    size_t j = (i + 1) % size;
    centroid_x += (((vector_t *)list_get(polygon, i))->x +
                   ((vector_t *)list_get(polygon, j))->x) *
                  (((vector_t *)list_get(polygon, i))->x *
                       ((vector_t *)list_get(polygon, j))->y -
                   ((vector_t *)list_get(polygon, j))->x *
                       ((vector_t *)list_get(polygon, i))->y);
    centroid_y += (((vector_t *)list_get(polygon, i))->y +
                   ((vector_t *)list_get(polygon, j))->y) *
                  (((vector_t *)list_get(polygon, i))->x *
                       ((vector_t *)list_get(polygon, j))->y -
                   ((vector_t *)list_get(polygon, j))->x *
                       ((vector_t *)list_get(polygon, i))->y);
  }
  centroid_x /= 6 * area;
  centroid_y /= 6 * area;

  return (vector_t){centroid_x, centroid_y};
}

void polygon_translate(list_t *polygon, vector_t translation) {
  size_t size = list_size(polygon);

  // Apply translation to each vertex
  for (size_t i = 0; i < size; ++i) {
    ((vector_t *)list_get(polygon, i))->x += translation.x;
    ((vector_t *)list_get(polygon, i))->y += translation.y;
  }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
  size_t size = list_size(polygon);

  for (size_t i = 0; i < size; ++i) {
    double x = ((vector_t *)list_get(polygon, i))->x;
    double y = ((vector_t *)list_get(polygon, i))->y;

    // Translate to point and rotate
    vector_t temp_vec = {.x = x - point.x, .y = y - point.y};
    temp_vec = vec_rotate(temp_vec, angle);

    // Translate back and store
    ((vector_t *)list_get(polygon, i))->x = temp_vec.x + point.x;
    ((vector_t *)list_get(polygon, i))->y = temp_vec.y + point.y;
  }
}

range_t polygon_proj(list_t *polygon, vector_t axis, bool normalize) {
  if (axis.x == 0 && axis.y == 0) {
    return (range_t){0, 0};
  }

  range_t proj_range = {INFINITY, -INFINITY};
  vector_t axis_normalized = normalize ? vec_normalize(axis) : axis;

  size_t size = list_size(polygon);
  for (size_t i = 0; i < size; ++i) {
    vector_t vert = *(vector_t *)list_get(polygon, i);
    double vert_proj = vec_scalar_proj(axis_normalized, vert, false);
    if (vert_proj < proj_range.min) {
      proj_range.min = vert_proj;
    }
    if (vert_proj > proj_range.max) {
      proj_range.max = vert_proj;
    }
  }
  return proj_range;
}