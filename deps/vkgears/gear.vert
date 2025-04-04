/*
 * Copyright © 2022 Collabora Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#version 450

layout(set = 0, binding = 0) uniform block {
    uniform mat4 projection;
};

layout(push_constant) uniform constants
{
    uniform mat4 modelview;
    vec3 material_color;
};

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

const vec3 L = normalize(vec3(5.0, 5.0, 10.0));

void main()
{
    vec3 N = normalize(mat3(modelview) * in_normal);

    float diffuse = max(0.0, dot(N, L));
    float ambient = 0.2;
    out_color = vec4((ambient + diffuse) * material_color, 1.0);

    gl_Position = projection * (modelview * in_position);
}
