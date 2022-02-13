#include "physics.h"

#define HEIGHTMAP_N	1024 // num data points per row
#define HEIGHTMAP_L	256  // side length of terrain
#define HEIGHTMAP_S	5    // terrain vertical scale

uint height_index(float pos_x, float pos_z)
{
	uint x = (pos_x / HEIGHTMAP_L) * HEIGHTMAP_N;
	uint z = (pos_z / HEIGHTMAP_L) * HEIGHTMAP_N;
	return (x * HEIGHTMAP_N) + z;
}

struct Heightmap
{
	float height[HEIGHTMAP_N * HEIGHTMAP_N];
};

float height(Heightmap* map, vec3 pos)
{
	return map->height[height_index(pos.x, pos.z)];
}
float height(Heightmap* map, vec2 pos)
{
	return map->height[height_index(pos.x, pos.y)];
}
vec3 terrain(Heightmap* map, vec2 pos)
{
	return vec3(pos.x, map->height[height_index(pos.x, pos.y)], pos.y);
}

struct Heightmap_Renderer
{
	Drawable_Mesh mesh;
	Shader shader;
	GLuint heights;
	GLuint albedos, normals, material;
};

void init(Heightmap_Renderer* renderer, Heightmap* heightmap, const char* path)
{
	load(&renderer->mesh, "assets/meshes/terrain.mesh");
	load(&renderer->shader, "assets/shaders/terrain.vert", "assets/shaders/terrain.frag");

	renderer->albedos  = load_texture_png("assets/textures/ground/albedos.png");
	renderer->normals  = load_texture_png("assets/textures/ground/normals.png");
	renderer->material = load_texture_png("assets/textures/ground/material.png");

	load_file_r32(path, heightmap->height, HEIGHTMAP_N);
	for (uint i = 0; i < HEIGHTMAP_N * HEIGHTMAP_N; i++) heightmap->height[i] *= HEIGHTMAP_S;

	glGenTextures(1, &renderer->heights);
	glBindTexture(GL_TEXTURE_2D, renderer->heights);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, HEIGHTMAP_N, HEIGHTMAP_N, 0, GL_RED, GL_FLOAT, heightmap->height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
void draw(Heightmap_Renderer* renderer, mat4 proj_view)
{
	bind_texture(renderer->heights , 2);
	bind_texture(renderer->albedos , 3);
	bind_texture(renderer->normals , 4);
	bind_texture(renderer->material, 5);
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	draw(renderer->mesh);
}