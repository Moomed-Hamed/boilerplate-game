#version 330 core

struct VS_OUT
{
	vec3 normal;
	vec3 frag_pos;
	vec3 color;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 world_position;
layout (location = 3) in vec3 scale;
layout (location = 4) in vec3 color;
layout (location = 5) in mat3 rotation;

uniform mat4 proj_view;

out VS_OUT vs_out;

void main()
{
	vs_out.normal   = (rotation * normal);
	vs_out.frag_pos = (rotation * (position * scale)) + world_position;
	vs_out.color    = color;

	gl_Position = proj_view * vec4(vs_out.frag_pos, 1.0);
}
