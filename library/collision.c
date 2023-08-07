#include "collision.h"

#include "list.h"
#include "polygon.h"
#include "vector.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct seperation_helper {
  bool seperated;
  double overlap;
} seperation_helper_t;

list_t *get_normalized_proj_axes(list_t *shape) {
  size_t num_verts = list_size(shape);
  list_t *axes = list_init(num_verts, free);
  for (size_t i = 0; i < num_verts; ++i) {
    // Calculate axis (unit vector perpendicular to edge)
    vector_t vert1 = *(vector_t *)list_get(shape, i);
    vector_t vert2 = *(vector_t *)list_get(shape, (i + 1) % list_size(shape));
    vector_t edge = vec_subtract(vert2, vert1);
    vector_t edge_normal = vec_rotate_90(edge, true);
    vector_t axis = vec_normalize(edge_normal);

    // Add axis to list
    vector_t *axis_ptr = malloc(sizeof(vector_t));
    *axis_ptr = axis;
    list_add(axes, axis_ptr);
  }
  return axes;
}

/**
 * Returns whether the projections of the given shapes onto the given axis are
 * separated. If so, the return also contains the amount of overlap the two
 * shapes have once projected onto the given axis.
 */
seperation_helper_t shapes_are_separated(vector_t normalized_axis,
                                         list_t *shape1, list_t *shape2) {
  range_t proj1 = polygon_proj(shape1, normalized_axis, false);
  range_t proj2 = polygon_proj(shape2, normalized_axis, false);
  seperation_helper_t seperated_info;
  seperated_info.seperated = proj1.max < proj2.min || proj2.max < proj1.min;
  double overlap;
  if (seperated_info.seperated == false) {
    if (proj1.min <= proj2.min) {
      if (proj1.max <= proj2.max) {
        // case 1
        overlap = proj1.max - proj2.min;
      } else {
        // case 3
        overlap = proj2.max - proj2.min;
      }
    } else {
      if (proj1.max >= proj2.max) {
        // case 4
        overlap = proj2.max - proj1.min;
      } else {
        // case 2
        overlap = proj1.max - proj1.min;
      }
    }
    seperated_info.overlap = overlap;
  }
  return seperated_info;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  list_t *axes1 = get_normalized_proj_axes(shape1);
  list_t *axes2 = get_normalized_proj_axes(shape2);

  collision_info_t ret_info;

  size_t num_axes1 = list_size(axes1);
  size_t num_axes2 = list_size(axes2);
  size_t num_axes = num_axes1 + num_axes2;
  ret_info.collided = true;
  double min = INFINITY;
  vector_t min_axis;
  for (size_t i = 0; i < num_axes; ++i) {
    list_t *axis_group = (i < num_axes1) ? axes1 : axes2;
    if (num_axes1 == 0) {
      num_axes1 = num_axes2;
      axis_group = axes2;
    }
    vector_t axis = *(vector_t *)list_get(axis_group, i % num_axes1);
    seperation_helper_t seperated_info =
        shapes_are_separated(axis, shape1, shape2);
    if (seperated_info.seperated == true) {
      ret_info.collided = false;
      break;
    } else {
      // finds the axis of minimum overlap
      if (min > seperated_info.overlap) {
        min_axis = axis;
        min = seperated_info.overlap;
      }
    }
  }

  if (ret_info.collided == true) {
    ret_info.axis = min_axis;
  }

  list_free(axes1);
  list_free(axes2);
  return ret_info;
}
