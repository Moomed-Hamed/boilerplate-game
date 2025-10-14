#include "drawer.h"

int main()
{
	console = Alloc(GameConsole, 1);

	GameWindow* window = Alloc(GameWindow, 1);
	window->init(1920, 1080);

	GeometryRenderer* geometry_renderer = Alloc(GeometryRenderer, 1);
	geometry_renderer->init();
	geometry_renderer->add_mesh("assets/meshes/SM/UV/sphere.mesh_uv");
	geometry_renderer->add_mesh("assets/meshes/SM/UV/cube.mesh_uv");
	geometry_renderer->add_mesh("assets/meshes/SM/UV/ammo.mesh_uv");

	window->timer.start();
	while (window->instance)
	{
		window->begin_frame(); // poll input, swap GL buffers

		// Check for exit
		if (window->instance == NULL)
		{
			console->add_entry((char*)"window.instance == NULL"); // | Window Shutdown!");
			break; // out of samsara
		}

		static float runningTime = 0;
		runningTime += 1.f / 60000;
		mat4 model[3] = { mat4(1), mat4(1) };
		model[0] = glm::rotate(mat4(1), 360.f * runningTime, vec3(0, 1, 0));

		model[1] = glm::translate(mat4(1), vec3(2, 0, 0));
		model[1] = glm::rotate(model[1], 360.f * runningTime, vec3(1, 1, 0));

		model[2] = glm::translate(mat4(1), vec3(-1, 0, 0));
		model[2] = glm::rotate(model[2], 360.f * runningTime, vec3(0, 1, 1));

		geometry_renderer->drawbuffer.add_instances(1, 1, model);
		geometry_renderer->drawbuffer.add_instances(2, 1, &model[1]);
		geometry_renderer->drawbuffer.add_instances(3, 1, &model[2]);

		// geometry
		geometry_renderer->draw(window);

		// gbuffer (direct lighting)
		window->draw_gbuf(geometry_renderer->camera.position);

		// draw console
		draw_console(console);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window->end_frame(); // calculate frame time, sleep
	}

	return 0;
}