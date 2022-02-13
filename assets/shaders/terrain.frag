#version 420 core

struct VS_OUT
{
	vec3 frag_pos;
	vec3 tex_coord;
};

in VS_OUT vs_out;

layout (location = 0) out vec4 frag_position;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_albedo;

layout (binding = 3) uniform sampler2D albedos;
layout (binding = 4) uniform sampler2D normals;
layout (binding = 5) uniform sampler2D material;

void main()
{
	vec2 tex = vs_out.tex_coord.xy * 64;

	vec3 normal = (texture(normals  , tex).rbg * 2) - 1;
	vec3 albedo =  texture(albedos  , tex).rgb;
	vec3 mat    =  texture(material , tex).rgb;

	frag_position = vec4(vs_out.frag_pos, mat.x); // metalness
	frag_normal   = vec4(normal         , mat.y); // roughness
	frag_albedo   = vec4(albedo         , mat.z); // ambient occlusion
}