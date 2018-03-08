#include <GLEW/glew.h>
#include <SDL/SDL.h>

#include "psim.hpp"
#include "assets.hpp"

#include <engine.hpp>
#include <window.hpp>
#include <camera.hpp>
#include <ui.hpp>
#include <graphics.hpp>

class sim_state : public ne::program_state {
public:

	ne::ortho_camera camera;
	ne::ortho_camera ui_camera;
	ne::drawing_shape quad;

	psim sim;

	sim_state::sim_state() {
		for (int i = 0; i < WORLD_SIZE * WORLD_SIZE; i++) {
			if (TERRAIN_AT(i) != WATER) {
				TERRAIN_AT(i) = GROUND;
			}
		}
		textures.world.parameters.are_pixels_in_memory = false;
		textures.world.render();
		camera.zoom = 2.0f;
		camera.target_chase_speed = 2.0f;
		camera.target_chase_aspect = 2.0f;
		quad.create();
		quad.make_quad();
		ne::listen([this](ne::mouse_wheel_message wheel) {
			ne::vector2f before = camera.size();
			camera.zoom += (float)wheel.wheel.y * 0.5f;
			ne::vector2f after = camera.size();
			ne::vector2f delta = after - before;
			camera.transform.position.x -= delta.x / 2.0f;
			camera.transform.position.y -= delta.y / 2.0f;
		});
		ne::listen([this](ne::keyboard_key_message key) {
			if (key.is_pressed && key.key == KEY_R) {
				camera.transform.position.xy = -100.0f;
			}
		});
	}

	void sim_state::update() {
		camera.transform.scale.xy = ne::window_size().to<float>();
		ui_camera.transform.scale.xy = ne::window_size().to<float>();
		if (ne::is_key_down(KEY_A) || ne::is_key_down(KEY_LEFT)) {
			camera.transform.position.x -= 20.0f;
		}
		if (ne::is_key_down(KEY_D) || ne::is_key_down(KEY_RIGHT)) {
			camera.transform.position.x += 20.0f;
		}
		if (ne::is_key_down(KEY_W) || ne::is_key_down(KEY_UP)) {
			camera.transform.position.y -= 20.0f;
		}
		if (ne::is_key_down(KEY_S) || ne::is_key_down(KEY_DOWN)) {
			camera.transform.position.y += 20.0f;
		}
		camera.update();
		sim.update();
		if (ne::is_key_down(KEY_F)) {
			debug.set(&fonts.debug, STRING(
				"Delta " << ne::delta() <<
				"\nFPS: " << ne::current_fps() <<
				"\nBears: " << sim.bears.size() <<
				"\nSeals: " << sim.seals.size() <<
				"\nSeals dead from hunger: " << sim.stats.seals.dead_from_hunger <<
				"\nBears dead from hunger: " << sim.stats.bears.dead_from_hunger <<
				"\nSeals born: " << sim.stats.seals.born <<
				"\nBears born: " << sim.stats.bears.born <<
				"\nSeals dead from age: " << sim.stats.seals.dead_from_age <<
				"\nBears dead from age: " << sim.stats.bears.dead_from_age <<
				"\nSeals dead randomly: " << sim.stats.seals.dead_randomly <<
				"\nBears dead randomly: " << sim.stats.bears.dead_randomly <<
				"\nSeals eaten by bears: " << sim.stats.seals_eaten_by_bears <<
				"\n\nYear: " << sim.year
			));
		} else {
			debug.set(&fonts.debug, STRING(
				"Delta " << ne::delta() <<
				"\nFPS: " << ne::current_fps() <<
				"\nBears: " << sim.bears.size() <<
				"\nSeals: " << sim.seals.size() <<
				"\n\nYear: " << sim.year
			));
		}
	}

	void sim_state::draw() {
		shaders.basic.bind();

		// World
		quad.bind();
		camera.bind();
		ne::transform3f view;
		view.position.xy = camera.xy();
		view.scale.xy = camera.size();
		sim.draw();

		// UI
		quad.bind();
		ui_camera.bind();
		view.position.xy = ui_camera.xy();
		view.scale.xy = ui_camera.size();
		ne::shader::set_color({ 0.0f, 0.0f, 0.0f, 1.0f });
		debug.draw(view);
	}

private:

	ne::debug_info debug;

};

void start() {
	glClearColor(0.7843137254901961f, 0.9215686274509804f, 1.0f, 1.0f);
	ne::maximise_window();
	load_assets();
	ne::set_update_sync(false);
	ne::set_max_update_count(MAX_TICKS_PER_DRAW);
	ne::set_swap_interval(ne::swap_interval::immediate);
	ne::swap_state<sim_state>();
}

void stop() {
	destroy_assets();
}

int main(int argc, char** argv) {
	ne::start_engine("PolarSim", 800, 600);
	return ne::enter_loop(start, stop);
}
