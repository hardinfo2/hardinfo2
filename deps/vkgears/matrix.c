/*
 * Hardinfo2 - System Information & Benchmark
 * Copyright hardinfo2 project, hwspeedy 2025
 * License: GPL2+
 *
 * Copyright (C) 1999-2001  Brian Paul
 * Copyright (C) 2010  Kristian Høgsberg
 * Copyright (C) 2010  Chia-I Wu
 * Copyright (C) 2023  Collabora Ltd
 *
 * was original MIT - but is relicensed
 */

#include "matrix.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

void
mat4_multiply(float m[4][4], const float n[4][4])
{
   float tmp[4][4];
   int i, j, k;

   for (j = 0; j < 4; j++) {
      for (i = 0; i < 4; i++) {
         float sum = 0.0f;
         for (k = 0; k < 4; k++)
            sum += mat4_get(m, i, k) * mat4_get(n, k, j);
         mat4_set(tmp, i, j, sum);
      }
   }
   memcpy(m, &tmp, sizeof tmp);
}

void
mat4_scale(float m[4][4], float x, float y, float z)
{
   float s[4][4] = {
      { x, 0, 0, 0 },
      { 0, y, 0, 0 },
      { 0, 0, z, 0 },
      { 0, 0, 0, 1 }
   };
   mat4_multiply(m, s);
}

void
mat4_rotate(float m[4][4], float angle, float x, float y, float z)
{
   double s, c;
#if HAVE_SINCOS
   sincos(angle, &s, &c);
#else
   s = sin(angle);
   c = cos(angle);
#endif
   float r[4][4] = {
      (double)x * x * (1 - c) + c,     (double)y * x * (1 - c) + z * s, (double)x * z * (1 - c) - y * s, 0,
      (double)x * y * (1 - c) - z * s, (double)y * y * (1 - c) + c,     (double)y * z * (1 - c) + x * s, 0,
      (double)x * z * (1 - c) + y * s, (double)y * z * (1 - c) - x * s, (double)z * z * (1 - c) + c,     0,
      { 0, 0, 0, 1 }
   };

   mat4_multiply(m, r);
}

void
mat4_translate(float m[4][4], float x, float y, float z)
{
   float t[4][4] = {
      { 1, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 0, 1, 0 },
      { x, y, z, 1 }
   };

   mat4_multiply(m, t);
}

void
mat4_identity(float m[4][4])
{
   float t[4][4] = {
      { 1.0, 0.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0, 0.0 },
      { 0.0, 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 0.0, 1.0 }
   };

   memcpy(m, t, sizeof(t));
}

void
mat4_transpose(float m[4][4])
{
   float t[4][4];
   for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
         t[i][j] = m[j][i];

   memcpy(m, t, sizeof(t));
}

void
mat4_invert(float m[4][4])
{
   float t[4][4];
   mat4_identity(t);

   // Extract and invert the translation part 't'. The inverse of a
   // translation matrix can be calculated by negating the translation
   // coordinates.
   for (int i = 0; i < 3; ++i)
      mat4_set(t, i, 3, -mat4_get(m, i, 3));

   // Invert the rotation part 'r'. The inverse of a rotation matrix is
   // equal to its transpose.
   for (int i = 0; i < 3; ++i)
      mat4_set(m, i, 3, 0.0f);
   mat4_transpose(m);

   // inv(m) = inv(r) * inv(t)
   mat4_multiply(m, t);
}

void
mat4_frustum_gl(float m[4][4], float l, float r, float b, float t, float n, float f)
{
   mat4_identity(m);

   float deltaX = r - l;
   float deltaY = t - b;
   float deltaZ = f - n;

   mat4_set(m, 0, 0, (2 * n) / deltaX);
   mat4_set(m, 1, 1, (2 * n) / deltaY);
   mat4_set(m, 0, 2, (r + l) / deltaX);
   mat4_set(m, 1, 2, (t + b) / deltaY);
   mat4_set(m, 2, 2, -(f + n) / deltaZ);
   mat4_set(m, 3, 2, -1.0f);
   mat4_set(m, 2, 3, -(2 * f * n) / deltaZ);
   mat4_set(m, 3, 3, 0.0f);
}

void
mat4_frustum_vk(float m[4][4], float l, float r, float b, float t, float n, float f)
{
   mat4_identity(m);

   float deltaX = r - l;
   float deltaY = t - b;
   float deltaZ = f - n;

   mat4_set(m, 0, 0, (2 * n) / deltaX);
   mat4_set(m, 1, 1, (-2 * n) / deltaY);
   mat4_set(m, 0, 2, (r + l) / deltaX);
   mat4_set(m, 1, 2, (t + b) / deltaY);
   mat4_set(m, 2, 2, f / (n - f));
   mat4_set(m, 3, 2, -1.0f);
   mat4_set(m, 2, 3, -(f * n) / deltaZ);
   mat4_set(m, 3, 3, 0.0f);
}

void
mat4_perspective_gl(float m[4][4], float fovy, float aspect,
                    float zNear, float zFar)
{
   mat4_identity(m);

   double sine, cosine, cotangent, deltaZ;
   float radians = fovy / 2 * M_PI / 180;

   deltaZ = zFar - zNear;
   sine = sin(radians);
   cosine = cos(radians);

   if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
      return;

   cotangent = cosine / sine;

   mat4_set(m, 0, 0, cotangent / aspect);
   mat4_set(m, 1, 1, cotangent);
   mat4_set(m, 2, 2, -(zFar + zNear) / deltaZ);
   mat4_set(m, 3, 2, -1.0f);
   mat4_set(m, 2, 3, -2 * (double)zNear * zFar / deltaZ);
   mat4_set(m, 3, 3, 0.0f);
}
