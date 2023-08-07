#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdbool.h>

/**
 * A real-valued 2-dimensional vector.
 * Positive x is towards the right; positive y is towards the top.
 * vector_t is defined here instead of vector.c because it is passed *by value*.
 */
typedef struct {
  double x;
  double y;
} vector_t;

/**
 * The zero vector, i.e. (0, 0).
 * "extern" declares this global variable without allocating memory for it.
 * You will need to define "const vector_t VEC_ZERO = ..." in vector.c.
 */
const extern vector_t VEC_ZERO;

/**
 * Adds two vectors.
 * Performs the usual componentwise vector sum.
 *
 * @param v1 the first vector
 * @param v2 the second vector
 * @return v1 + v2
 */
vector_t vec_add(vector_t v1, vector_t v2);

/**
 * Subtracts two vectors.
 * Performs the usual componentwise vector difference.
 *
 * @param v1 the first vector
 * @param v2 the second vector
 * @return v1 - v2
 */
vector_t vec_subtract(vector_t v1, vector_t v2);

/**
 * Computes the additive inverse a vector.
 * This is equivalent to multiplying by -1.
 *
 * @param v the vector whose inverse to compute
 * @return -v
 */
vector_t vec_negate(vector_t v);

/**
 * Multiplies a vector by a scalar.
 * Performs the usual componentwise product.
 *
 * @param scalar the number to multiply the vector by
 * @param v the vector to scale
 * @return scalar * v
 */
vector_t vec_multiply(double scalar, vector_t v);

/**
 * Computes the dot product of two vectors.
 * See https://en.wikipedia.org/wiki/Dot_product#Algebraic_definition.
 *
 * @param v1 the first vector
 * @param v2 the second vector
 * @return v1 . v2
 */
double vec_dot(vector_t v1, vector_t v2);

/**
 * Computes the cross product of two vectors,
 * which lies along the z-axis.
 * See https://en.wikipedia.org/wiki/Cross_product#Computing_the_cross_product.
 *
 * @param v1 the first vector
 * @param v2 the second vector
 * @return the z-component of v1 x v2
 */
double vec_cross(vector_t v1, vector_t v2);

/**
 * Rotates a vector by an angle around (0, 0).
 * The angle is given in radians.
 * Positive angles are counterclockwise, according to the right hand rule.
 * See https://en.wikipedia.org/wiki/Rotation_matrix.
 * (You can derive this matrix by noticing that rotation by a fixed angle
 * is linear and then computing what it does to (1, 0) and (0, 1).)
 *
 * @param v the vector to rotate
 * @param angle the angle to rotate the vector
 * @return v rotated by the given angle
 */
vector_t vec_rotate(vector_t v, double angle);

/**
 * Rotates a vector by an angle of 90 degrees around (0, 0) in the given
 * direction.
 *
 * @param v the vector to rotate
 * @param counterclockwise whether to rotate the vector in the counterclockwise
 * direction
 * @return v rotated by 90 degrees in the given direction
 */
vector_t vec_rotate_90(vector_t v, bool counterclockwise);

/**
 * Calculates Euclidean Distance between vectors.
 *
 * @param v1 the first vector
 * @param v2 the second vector
 * @return distance
 */
double vec_dist(vector_t v1, vector_t v2);

/**
 * Calculates the Euclidean norm of a vector.
 *
 * @param v the vector whose norm to compute
 * @return ||v||
 */
double vec_norm(vector_t v);

/**
 * Calculates the normalized vector of a vector.
 * This is equivalent to dividing by the norm.
 * Returns the zero vector if the given vector is the zero vector.
 *
 * @param v the vector whose normalized vector to compute
 * @return v / ||v||, or (0, 0) if ||v|| = 0
 */
vector_t vec_normalize(vector_t v);

/**
 * Calculates the scalar projection of a vector onto an axis.
 * If normalize is false, the axis is assumed to have norm 1.
 * Returns 0 if the axis is the zero vector.
 *
 * @param axis the axis onto which the vector is projected
 * @param v the vector to project onto the axis
 * @param normalize whether to normalize the axis
 * @return v . axis / ||axis||
 */
double vec_scalar_proj(vector_t axis, vector_t v, bool normalize);

/**
 * Calculates the vector projection of a vector onto an axis.
 * If normalize is false, the axis is assumed to have norm 1.
 * Returns the zero vector if the axis is the zero vector.
 *
 * @param axis the axis onto which the vector is projected
 * @param v the vector to project onto the axis
 * @param normalize whether to normalize the axis
 * @return (v . axis) / (axis . axis) * axis
 */
vector_t vec_vec_proj(vector_t axis, vector_t v, bool normalize);

#endif // #ifndef __VECTOR_H__
