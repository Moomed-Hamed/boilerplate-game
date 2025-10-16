#include "loader.h"

/* GeometryRenderer : drawing geometry to the G-Buffer */

// instance data might not be in the same order as geometry data.
// the number of instances of each mesh might vary every frame.
// Therefore :
// Each mesh id can only be updated once per frame because
// while geometry data is static, we get instance data
// out of order every frame and we don't know how big each
// block will be. so for now, for each mesh must only call
// update_mesh(mesh_id, instances) a max of once per frame
// TODO : should we check this & throw an error if called twice?
struct DrawBuffer
{	
	struct {
		uint mesh_id;

		uint num_indices;
		uint index_offset; // IN BYTES : num_indices * sizeof(uint)
		uint base_vertex;  // IN VERTS : mesh_offset / sizeof(Vertex)

		uint num_instances;
		uint base_instance; // IN INSTANCES
	} mesh_info[MAX_MESHES];

	// OpenGL gpu buffers
	GLuint geom_buffer; // handle to vertex buffer
	GLuint indx_buffer; // handle to index 
	GLuint inst_buffer; // handle to instance buffer

	// runtime buffer info
	uint max_buffer_size;

	uint num_meshes;
	uint total_instances; // total number of instances currently stored

	uint geom_size;
	uint indx_size;
	uint inst_size;

	uint geom_offset, indx_offset;

	void init(GLuint vao, uint buffer_size = KiloByte(256)) {

		// this size is for : vertex buffer, index buffer, instance-data buffer
		max_buffer_size = buffer_size;

		// generate gpu buffers : mesh vertices, mesh indices, & instance data
		glGenBuffers(1, &geom_buffer);
		glGenBuffers(1, &indx_buffer);
		glGenBuffers(1, &inst_buffer);

		// IMPORTANT : bind the vao *before* binding anything else!
		glBindVertexArray(vao);

		// define mesh layout : vec3 position, vec3 normal, vec2 uv ---------------------------

		struct MeshVertex {
			vec3 p, n;
			vec2 uv;
		};

		// gpu buffer for mesh vertices
		glBindBuffer(GL_ARRAY_BUFFER, geom_buffer);
		glBufferData(GL_ARRAY_BUFFER, max_buffer_size, NULL, GL_STATIC_DRAW);

		// this code tells opengl about the layout of mesh vertex data
		uint stride = sizeof(MeshVertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(vec3)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(vec3) * 2));
		glEnableVertexAttribArray(2);

		// ------------------------------------------------------------------------------------

		// gpu buffer for per-mesh instance data
		glBindBuffer(GL_ARRAY_BUFFER, inst_buffer);
		glBufferData(GL_ARRAY_BUFFER, max_buffer_size, NULL, GL_DYNAMIC_DRAW);

		// define per-mesh instance-data layout
		uint num_vertex_attribs = 3;
		for (uint i = num_vertex_attribs; i < 4 + num_vertex_attribs; i++)
		{
			glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(vec4) * (i - num_vertex_attribs)));
			glEnableVertexAttribArray(i);
			glVertexAttribDivisor(i, 1);
		}

		// gpu index buffer : ordered uints that reference vertices for building meshes
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indx_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_buffer_size, NULL, GL_STATIC_DRAW);

		// log
		char msg[62] = {};
		snprintf(msg, 62, "Init Draw Buffer | Size : [%d], [%d] bytes total", max_buffer_size, max_buffer_size * 3);
		console->add_entry(msg, SUCCESS, RNDR);
	}

	// This function appends mesh vertices & indices to the appropriate gpu buffers
	void add_geometry(uint mesh_id, Mesh_Data mesh_data) {

		// for storing vertex data in the correct format
		// so it can go into a gpu buffer to be drawn
		struct MeshVertex {
			vec3 position, normal;
			vec2 uv;
		};

		uint num_vertices = mesh_data.num_vertices;
		uint num_indices  = mesh_data.num_indices;

		uint vert_data_size  = num_vertices * sizeof(MeshVertex);
		uint index_data_size = num_indices  * sizeof(uint);

		// Ensure new mesh data will fit in gpu buffers!

		if (geom_offset + vert_data_size > max_buffer_size) {
			out("Geom VBO size exceeded! - " << geom_offset + vert_data_size);
			stop;
		}

		if (indx_offset + index_data_size > max_buffer_size) {
			out("Index VBO size exceeded! - " << indx_offset + index_data_size);
			stop;
		}

		// update geometry buffer

		MeshVertex* mesh_verts = Alloc(MeshVertex, num_vertices);

		// assemble vertex buffer to be copied to gpu buffer
		for (uint i = 0; i < num_vertices; i++)
			mesh_verts[i] = MeshVertex{ mesh_data.positions[i], mesh_data.normals[i], mesh_data.uvs[i] };

		// upload mesh data to gpu buffer
		glBindBuffer(GL_ARRAY_BUFFER, geom_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, geom_offset, vert_data_size, mesh_verts);

		free(mesh_verts);

		// update index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indx_buffer);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indx_offset, index_data_size, mesh_data.indices);

		// update mesh info
		uint mesh_index = num_meshes++;
		mesh_info[mesh_index].mesh_id       = mesh_id;
		mesh_info[mesh_index].base_vertex   = geom_offset / sizeof(MeshVertex);
		mesh_info[mesh_index].num_indices   = num_indices;
		mesh_info[mesh_index].index_offset  = indx_offset;

		// update offsets
		geom_offset += vert_data_size;
		indx_offset += index_data_size;
	}

	// This function appends per-mesh instance data to the appropriate gpu buffers
	void add_instances(uint mesh_id, uint num_instances, mat4* instance_data)
	{
		glBindBuffer(GL_ARRAY_BUFFER, inst_buffer);

		uint instance_data_size = num_instances   * sizeof(mat4); // UNIT : BYTES
		uint instance_offset    = total_instances * sizeof(mat4); // UNIT : BYTES

		glBufferSubData(GL_ARRAY_BUFFER, instance_offset, instance_data_size, instance_data);

		// update corresponding mesh info
		for (uint i = 0; i < MAX_MESHES; i++)
		{
			if (mesh_info[i].mesh_id == mesh_id)
			{
				mesh_info[i].num_instances = num_instances;
				mesh_info[i].base_instance = total_instances; // UNIT : INSTANCES
				total_instances += num_instances;
				return;
			}
		}

		out("ERROR : Instances do not match a stored mesh!");
		stop;
	}
};

// list of meshes to be drawn + instance information
struct DrawList
{
	uint meshlist[MAX_MESHES]; // mesh ids in the list

	// params for issuing a draw call on an instanced mesh, in mesh VBO order
	struct {
		uint num_indices;
		uint index_offset; // IN BYTES : num_indices * sizeof(uint)
		uint base_vertex;  // IN VERTS : mesh_offset / sizeof(Vertex)
		uint num_instances;
		uint base_instance; // IN INSTANCES
	} mesh_params[MAX_MESHES];

	void update(DrawBuffer* db)
	{
		*this = {}; // zero memory
		db->total_instances = 0;

		for (uint i = 0; i < MAX_MESHES; i++)
		{
			if (db->mesh_info[i].mesh_id == 0)
				continue;

			meshlist[i] = db->mesh_info[i].mesh_id;

			mesh_params[i].num_indices   = db->mesh_info[i].num_indices;
			mesh_params[i].index_offset  = db->mesh_info[i].index_offset;
			mesh_params[i].base_vertex   = db->mesh_info[i].base_vertex;
			mesh_params[i].num_instances = db->mesh_info[i].num_instances;
			mesh_params[i].base_instance = db->mesh_info[i].base_instance;
		}
	}
};

struct GeometryRenderer {
	// geometry pass params
	uint VAO, texture;
	ShaderProgram shader; // geometry shader
	Camera camera; // 3d camera

	// caching & loading meshes
	MeshLoader meshloader;
	DrawBuffer drawbuffer;
	DrawList   drawlist;

	void init();
	void add_mesh(const char* filepath);
	void draw(GameWindow* window); // geometry pass!
};

void GeometryRenderer::init()
{
	shader.create("assets/shaders/geom.vert", "assets/shaders/geom.frag");

	glGenVertexArrays(1, &VAO);
	drawbuffer.init(VAO);

	// Load texture
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Texture wrapping/filtering options (on currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load & generate texture
	int width, height, num_channels;
	unsigned char* data = stbi_load("assets/textures/default.jpg", &width, &height, &num_channels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else out("Failed to load texture");

	stbi_image_free(data);

	console->add_entry((char*)"Init GeometryRenderer", SUCCESS, RNDR);
}
void GeometryRenderer::add_mesh(const char* filepath)
{
	uint mesh_id = meshloader.load_mesh(filepath);

	Mesh_Data mesh_data = meshloader.load_mesh_data(mesh_id);

	drawbuffer.add_geometry(mesh_id, mesh_data);

	mesh_data.release();

	// log
	char msg[62] = {};
	snprintf(msg, 62, "Add Mesh, id[%d], path[%s]", mesh_id, filepath);
	console->add_entry(msg, SUCCESS, RNDR);
}
void GeometryRenderer::draw(GameWindow* window)
{
	camera.update_dir(window->mouse.dx, window->mouse.dy, 1.f / 600);
	if (window->keys.W.is_pressed) camera.position += camera.front * (1.f / 60);
	if (window->keys.S.is_pressed) camera.position -= camera.front * (1.f / 60);
	if (window->keys.D.is_pressed) camera.position += camera.right * (1.f / 60);
	if (window->keys.A.is_pressed) camera.position -= camera.right * (1.f / 60);

	// Create projection-view matrix for drawing
	float fov = 45, draw_distance = 256;
	mat4 proj = perspective(fov, (float)window->screen_width / window->screen_height, 0.1f, draw_distance);
	mat4 proj_view = proj * glm::lookAt(camera.position, camera.position + camera.front, camera.up);

	glBindFramebuffer(GL_FRAMEBUFFER, window->gbuf.FBO);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(VAO);
	shader.bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	uint pv = glGetUniformLocation(shader.id, "proj_view");
	glUniformMatrix4fv(pv, 1, GL_FALSE, (float*)&proj_view);

	//out("drawing total meshes : " << gpu_buffer.draw_list.total_meshes);

	drawlist.update(&drawbuffer);

	// draw each instanced mesh
	for (uint i = 0; i < MAX_MESHES; i++)
	{
		if (drawlist.meshlist[i] == 0)
			continue;

		uint num_indices     = drawlist.mesh_params[i].num_indices;
		uint index_offset    = drawlist.mesh_params[i].index_offset;
		uint vertex_offset   = drawlist.mesh_params[i].base_vertex;
		uint num_instances   = drawlist.mesh_params[i].num_instances;
		uint instance_offset = drawlist.mesh_params[i].base_instance;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT,
			(void*)index_offset, num_instances, vertex_offset, instance_offset);
	}
}