#version 330 core

struct VS_OUT {
	vec2 uv;
	vec3 view_position;
};

layout (location = 0) in vec2 vertex_position; // screen quad
layout (location = 1) in vec2 uv;

uniform vec3 view_pos;

out VS_OUT vs_out;

void main()
{
	vs_out.uv   = uv;
	vs_out.view_position = view_pos;

	gl_Position = vec4(vertex_position, 0.0, 1.0);
}