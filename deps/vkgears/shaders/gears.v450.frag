#version 450

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec4 v_color;
layout (location = 2) in vec3 v_fragPos;
layout (location = 3) in vec3 v_lightDir;

layout (location = 0) out vec4 outFragColor;

void main()
{
  float ambient = 0.1;
  float diff = max(dot(v_normal, v_lightDir), 0.0);

  vec4 finalColor = (ambient + diff) * v_color;

  outFragColor = vec4(finalColor.rgb, v_color.a);
}