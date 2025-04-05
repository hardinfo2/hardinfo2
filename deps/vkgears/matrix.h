#ifndef MATRIX_H
#define MATRIX_H

#include <assert.h>

/*
 * 4x4 matrix routines. Works on column-major data, suitable for usage
 * with OpenGL.
 *
 * memory layout:
 *
 * [[m00 m10 m20 m30]
 *  [m01 m11 m21 m31]
 *  [m02 m12 m22 m32]
 *  [m03 m13 m23 m33]]
 *
 * m(i,j) = m[j][i]
 */

static inline float
mat4_get(const float m[4][4], unsigned i, unsigned j)
{
    assert(i < 4);
    assert(j < 4);
    return m[j][i];
}

static inline void
mat4_set(float m[4][4], unsigned i, unsigned j, float v)
{
    assert(i < 4);
    assert(j < 4);
    m[j][i] = v;
}

/**
 * Multiplies two 4x4 matrices.
 *
 * The result is stored in matrix m.
 *
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
void
mat4_multiply(float m[4][4], const float n[4][4]);

/**
 * Scales a 4x4 matrix.
 *
 * @param[in,out] m the matrix to scale
 * @param x the scale-factor for the x-axis
 * @param y the scale-factor for the y-axis
 * @param z the scale-factor for the z-axis
 */
void
mat4_scale(float m[4][4], float x, float y, float z);

/**
 * Rotates a 4x4 matrix.
 *
 * @param[in,out] m the matrix to rotate
 * @param angle the angle to rotate
 * @param x the x component of the direction to rotate to
 * @param y the y component of the direction to rotate to
 * @param z the z component of the direction to rotate to
 */
void
mat4_rotate(float m[4][4], float angle, float x, float y, float z);

/**
 * Translates a 4x4 matrix.
 *
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
void
mat4_translate(float m[4][4], float x, float y, float z);

/**
 * Creates an identity 4x4 matrix.
 *
 * @param m the matrix make an identity matrix
 */
void
mat4_identity(float m[4][4]);

/**
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
void
mat4_transpose(float m[4][4]);

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
void
mat4_invert(float m[4][4]);

/**
 * Calculate an OpenGL frustum projection transformation.
 *
 * @param m the matrix to save the transformation in
 * @param l the left plane distance
 * @param r the right plane distance
 * @param b the bottom plane distance
 * @param t the top plane distance
 * @param n the near plane distance
 * @param f the far plane distance
 */
void
mat4_frustum_gl(float m[4][4], float l, float r, float b, float t, float n, float f);

/**
 * Calculate a Vulkan frustum projection transformation.
 *
 * @param m the matrix to save the transformation in
 * @param l the left plane distance
 * @param r the right plane distance
 * @param b the bottom plane distance
 * @param t the top plane distance
 * @param n the near plane distance
 * @param f the far plane distance
 */
void
mat4_frustum_vk(float m[4][4], float l, float r, float b, float t, float n, float f);

/**
 * Calculate a perspective projection transformation.
 *
 * @param m the matrix to save the transformation in
 * @param fovy the field of view in the y direction
 * @param aspect the view aspect ratio
 * @param zNear the near clipping plane
 * @param zFar the far clipping plane
 */
void
mat4_perspective_gl(float m[4][4], float fovy, float aspect,
                    float zNear, float zFar);

#endif /* MATRIX_H */
