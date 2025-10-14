#include "window.h"

//> Load meshes, textures, audio, animations
//> Store into well-defined buffers for the game

struct Mesh_Data
{
	uint num_vertices, num_indices;

	vec3* positions, *normals;
	vec2* uvs;
	uint* indices;

	void load(const char* path)
	{
		FILE* mesh_file = fopen(path, "rb");
		if (!mesh_file) { print("could not open model file: %s\n", path); stop; return; }

		fread(&num_vertices, sizeof(uint), 1, mesh_file);
		fread(&num_indices , sizeof(uint), 1, mesh_file);

		positions = (vec3*)calloc(num_vertices, sizeof(vec3));
		normals   = (vec3*)calloc(num_vertices, sizeof(vec3));
		uvs       = (vec2*)calloc(num_vertices, sizeof(vec2));
		indices   = (uint*)calloc(num_indices , sizeof(uint));

		fread(positions, sizeof(vec3), num_vertices, mesh_file);
		fread(normals  , sizeof(vec3), num_vertices, mesh_file);
		fread(uvs      , sizeof(vec2), num_vertices, mesh_file);
		fread(indices  , sizeof(uint), num_indices , mesh_file);

		fclose(mesh_file);
	}
	void release()
	{
		free(positions);
		free(normals);
		free(uvs);
		free(indices);
	}
};

const uint MAX_MESHES = 16;
const uint MAX_MESH_FILEPATH_LENGTH = 64;

/* MeshLoader : loading & caching mesh data from disk
* 
* METHOD : cache_mesh
* - reads mesh metadata from file, stores metadata & assigns mesh a unique id
* - stores size of mesh vertices & indices
* - *does not* load or store vertex/index data
* 
* METHOD : load_mesh_data
* - reads from file the actual vertices & indices for a mesh
* - returns the loaded data in a Mesh_Data struct
* - Mesh_Data returned as dynamically allocated array that must be freed later
*/
struct MeshLoader {
	uint num_cached, total_cached; // for next id

	struct MeshInfo {
		uint id, size, is_cached;
		char filepath[MAX_MESH_FILEPATH_LENGTH];
	} meshes[MAX_MESHES];

	uint load_mesh(const char* filepath) // returns mesh_id
	{
		// check if previously cached
		for (uint i = 0; i < MAX_MESHES; i++)
		{
			if (strcmp(meshes[i].filepath, filepath) == 0) // 0 = strings match!
			{
				out(filepath << " already cached!");
				return meshes[i].id; // mesh is already cached!
			}
		}

		// if not, then load from disk & add it to cache
		uint num_vertices = 0, num_indices = 0;

		FILE* mesh_file = fopen(filepath, "rb");
		if (!mesh_file) { print("could not open model file: %s\n", filepath); stop; return 0; }

		// do we need these here or should they be stored in vertex buffer?
		// if they're here it should only be for calculating vertex & index buffer sizes
		fread(&num_vertices, sizeof(uint), 1, mesh_file);
		fread(&num_indices , sizeof(uint), 1, mesh_file);

		fclose(mesh_file);

		// look for empty spot
		for (uint i = 0; i < MAX_MESHES; i++)
		{
			if (meshes[i].id == 0)
			{
				uint mesh_filepath_length = strlen(filepath);
				if (mesh_filepath_length < MAX_MESH_FILEPATH_LENGTH)
					strcpy(meshes[i].filepath, filepath);
				else {
					out("ERROR : max filepath exceeded!\n FILENAME : " << filepath);
					out("\n SIZE : " << mesh_filepath_length << " | MAX : " << MAX_MESH_FILEPATH_LENGTH);
					stop;
				}

				meshes[i].id = total_cached + 1; // avoid NULL value
				meshes[i].is_cached = true;

				num_cached++;
				total_cached++;

				return meshes[i].id;
			}
		}

		out("ERROR : Cannot Cache : [" << filepath << "]!");
	}
	Mesh_Data load_mesh_data(uint mesh_id)
	{
		Mesh_Data mesh_data = {};

		for (uint i = 0; i < MAX_MESHES; i++)
		{
			if (meshes[i].id == mesh_id)
			{
				mesh_data.load(meshes[i].filepath);
				return mesh_data;
			}
		}

		return mesh_data; // fields will be NULL if not loaded
	}
};

namespace MeshFactory {
	Mesh_Data generate_cube(vec3 scale = vec3(1))
	{
		float positions[] = {
			// front face
			-0.5f, -0.5f,  0.5f, // 0
			 0.5f, -0.5f,  0.5f, // 1
			 0.5f,  0.5f,  0.5f, // 2
			-0.5f,  0.5f,  0.5f, // 3

			// back face
			 0.5f, -0.5f, -0.5f, // 4
			-0.5f, -0.5f, -0.5f, // 5
			-0.5f,  0.5f, -0.5f, // 6
			 0.5f,  0.5f, -0.5f, // 7

			// left face
			-0.5f, -0.5f, -0.5f, // 8
			-0.5f, -0.5f,  0.5f, // 9
			-0.5f,  0.5f,  0.5f, //10
			-0.5f,  0.5f, -0.5f, //11

			// right face
			 0.5f, -0.5f,  0.5f, //12
			 0.5f, -0.5f, -0.5f, //13
			 0.5f,  0.5f, -0.5f, //14
			 0.5f,  0.5f,  0.5f, //15

			// top face
			-0.5f,  0.5f,  0.5f, //16
			 0.5f,  0.5f,  0.5f, //17
			 0.5f,  0.5f, -0.5f, //18
			-0.5f,  0.5f, -0.5f, //19

			// bottom face
			-0.5f, -0.5f, -0.5f, //20
			 0.5f, -0.5f, -0.5f, //21
			 0.5f, -0.5f,  0.5f, //22
			-0.5f, -0.5f,  0.5f  //23
		};

		float normals[] = {
			// front (z+)
			0.0f,  0.0f,  1.0f,
			0.0f,  0.0f,  1.0f,
			0.0f,  0.0f,  1.0f,
			0.0f,  0.0f,  1.0f,

			// back (z-)
			0.0f,  0.0f, -1.0f,
			0.0f,  0.0f, -1.0f,
			0.0f,  0.0f, -1.0f,
			0.0f,  0.0f, -1.0f,

			// left (x-)
		  -1.0f,  0.0f,  0.0f,
		  -1.0f,  0.0f,  0.0f,
		  -1.0f,  0.0f,  0.0f,
		  -1.0f,  0.0f,  0.0f,

		  // right (x+)
		  1.0f,  0.0f,  0.0f,
		  1.0f,  0.0f,  0.0f,
		  1.0f,  0.0f,  0.0f,
		  1.0f,  0.0f,  0.0f,

		  // top (y+)
		  0.0f,  1.0f,  0.0f,
		  0.0f,  1.0f,  0.0f,
		  0.0f,  1.0f,  0.0f,
		  0.0f,  1.0f,  0.0f,

		  // bottom (y-)
		  0.0f, -1.0f,  0.0f,
		  0.0f, -1.0f,  0.0f,
		  0.0f, -1.0f,  0.0f,
		  0.0f, -1.0f,  0.0f
		};

		float uvs[] = {
			// front
			0, 0,  1, 0,  1, 1,  0, 1,
			// back
			0, 0,  1, 0,  1, 1,  0, 1,
			// left
			0, 0,  1, 0,  1, 1,  0, 1,
			// right
			0, 0,  1, 0,  1, 1,  0, 1,
			// top
			0, 0,  1, 0,  1, 1,  0, 1,
			// bottom
			0, 0,  1, 0,  1, 1,  0, 1
		};

		uint indices[] = {
			// front
			0, 1, 2,  2, 3, 0,
			// back
			4, 5, 6,  6, 7, 4,
			// left
			8, 9,10, 10,11, 8,
			// right
			12,13,14, 14,15,12,
			// top
			16,17,18, 18,19,16,
			// bottom
			20,21,22, 22,23,20
		};

		return Mesh_Data{}; // TODO : finish this function
	}
}