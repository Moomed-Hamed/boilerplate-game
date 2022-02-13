#include "player.h"

int main()
{
	Window   window = {};
	Mouse    mouse  = {};
	Keyboard keys   = {};

	init_window(&window, 1920, 1080, "boilerplate");
	init_keyboard(&keys);

	Heightmap* heightmap = Alloc(Heightmap, 1);
	Heightmap_Renderer* heightmap_renderer = Alloc(Heightmap_Renderer, 1);
	init(heightmap_renderer, heightmap, "assets/textures/heightmap.r32");

	Player* player = Alloc(Player, 1); init(player);

	Particle_Emitter* emitter = Alloc(Particle_Emitter, 1);
	Particle_Renderer* particle_renderer = Alloc(Particle_Renderer, 1);
	init(particle_renderer);

	Physics_Colliders* colliders = Alloc(Physics_Colliders, 1);
	colliders->dynamic.cubes     [0] = { {1, .5, 3}, {}, {}, 1, vec3(1) };
	colliders->dynamic.cylinders [0] = { {5, .5, 3}, {}, {}, 1, 1, .5 };
	colliders->dynamic.spheres   [0] = { {3, .5, 3}, {}, {}, 1, .5 };

	Physics_Renderer* physics_renderer = Alloc(Physics_Renderer, 1);
	init(physics_renderer);

	G_Buffer g_buffer = make_g_buffer(window);
	Shader lighting_shader = make_lighting_shader();
	mat4 proj = glm::perspective(FOV, (float)window.screen_width / window.screen_height, 0.1f, DRAW_DISTANCE);

	// frame timer
	float frame_time = 1.f / 45;
	int64 target_frame_milliseconds = frame_time * 1000.f; // seconds * 1000 = milliseconds
	Timestamp frame_start = get_timestamp(), frame_end;

	while (1)
	{
		update_window(window);
		update_mouse(&mouse, window);
		update_keyboard(&keys, window);

		if (keys.ESC.is_pressed) break;

		if (keys.G.is_pressed)
		{
			player->eyes.trauma = 1;
			emit_explosion(emitter, player->eyes.position + 14.f * player->eyes.front);
		}

		static float a = 0; a += frame_time;
		if (a > .4) { a = 0; emit_fire(emitter, terrain(heightmap, vec2(9, 6))); }

		// game updates
		update(player , frame_time, heightmap, keys, mouse);
		update(emitter, heightmap, frame_time, vec3(0));

		// renderer updates
		update(physics_renderer  , colliders);
		update(particle_renderer , emitter);

		// geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		mat4 proj_view = proj * lookAt(player->eyes.position, player->eyes.position + player->eyes.front, player->eyes.up);
		
		draw(physics_renderer   , proj_view);
		draw(particle_renderer  , proj_view);
		draw(heightmap_renderer , proj_view);

		// lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(lighting_shader);
		set_vec3(lighting_shader, "view_pos", player->eyes.position);
		draw(g_buffer);

		//frame time
		frame_end = get_timestamp();
		int64 milliseconds_elapsed = calculate_milliseconds_elapsed(frame_start, frame_end);

		//print("frame time: %02d ms | fps: %06f\n", milliseconds_elapsed, 1000.f / milliseconds_elapsed);
		if (target_frame_milliseconds > milliseconds_elapsed) // frame finished early
		{
			os_sleep(target_frame_milliseconds - milliseconds_elapsed);
		}
		
		frame_start = frame_end;
	}

	shutdown_window();
	return 0;
}