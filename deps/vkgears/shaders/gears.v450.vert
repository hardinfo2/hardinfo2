#version 450

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_color;

layout (binding = 0) uniform UBO
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 lightpos;
} ubo;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec4 v_color;
layout (location = 2) out vec3 v_fragPos;
layout (location = 3) out vec3 v_lightDir;

void main()
{
	mat4 modelView = ubo.view * ubo.model;
	vec4 pos = modelView * vec4(a_pos,1.0);

	mat3 normalMatrix = transpose(inverse(mat3(modelView)));

	v_normal = normalize(normalMatrix * a_normal);
	v_color = a_color;
	v_fragPos = vec3(ubo.model * vec4(a_pos,1.0));
	v_lightDir = normalize(ubo.lightpos - v_fragPos);

	gl_Position = ubo.projection * pos;
}
