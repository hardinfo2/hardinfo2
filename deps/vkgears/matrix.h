#ifndef MATRIX_H
#define MATRIX_H

/**
 * Multiplies two 4x4 matrices.
 *
 * The result is stored in matrix m.
 *
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
void
mat4_multiply(float *m, const float *n);

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
mat4_rotate(float *m, float angle, float x, float y, float z);

/**
 * Translates a 4x4 matrix.
 *
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
void
mat4_translate(float *m, float x, float y, float z);

/**
 * Creates an identity 4x4 matrix.
 *
 * @param m the matrix make an identity matrix
 */
void
mat4_identity(float *m);

/**
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
void
mat4_transpose(float *m);

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
void
mat4_invert(float *m);

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
mat4_frustum_gl(float *m, float l, float r, float b, float t, float n, float f);

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
mat4_frustum_vk(float *m, float l, float r, float b, float t, float n, float f);

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
mat4_perspective_gl(float *m, float fovy, float aspect,
                    float zNear, float zFar);

#endif /* MATRIX_H */
