#pragma once

#include "assets.hpp"
#include <timer.hpp>

#define WORLD_SIZE    textures.world.size.x
#define TERRAIN_AT(I) textures.world.pixels[I]

#define BEAR     0xFFFFFF00
#define SEAL     0xFFFF0000
#define GROUND   0xFF338844
#define WATER    0xFFC8EBFF
#define ICE      0xFFEEEEEE

struct cell_data {
	int8 direction = 0;
	uint8 age = 0;
	float hunger = 0;
	uint32 animal = 0;
};

struct look_info {
	ne::vector2i coord;
	int left_x = 0;
	int top_y = 0;
	int right_x = 0;
	int bottom_y = 0;
};

struct export_data {
	uint8 age_dead = 0;
	float hunger_dead = 0.0f;
	uint32 animal = 0;
	int time_of_death = 0;
};

#define TICKS_PER_YEAR				100
#define HUNGER_TO_DIE_IN_A_YEAR		0.0075f
#define RANDOM_DIRECTION			rng_direction(rng)
#define SWAP_AND_POP(V, I)			(V)[I] = (V).back(); (V).pop_back();
#define SET_ANIMAL(V)				animals = &V; to_add = &V##_to_add; stats.animals = &stats.##V;
#define LOG_ENABLED					0
#define MAX_TICKS_PER_DRAW			1

#define INITIAL_BEAR_COUNT			1000
#define RANDOM_BEAR_DEATH_CHANCE	0.05f
#define BEAR_MAX_AGE				30
#define BEAR_BREED_AGE				5
#define BEAR_BREED_PROBABILITY		0.1f

#define INITIAL_SEAL_COUNT			50000
#define RANDOM_SEAL_DEATH_CHANCE	0.1f
#define SEAL_MAX_AGE				25
#define SEAL_BREED_AGE				3
#define SEAL_BREED_PROBABILITY		0.5f

struct psim {

	ne::texture ani;

	struct {
		struct animal_stats {
			int dead_from_hunger = 0;
			int born = 0;
			int dead_from_age = 0;
			int dead_randomly = 0;
		} bears, seals;
		animal_stats* animals = nullptr;
		int seals_eaten_by_bears = 0;
	} stats;

#if LOG_ENABLED
	std::vector<export_data> log;
#endif

	int year = 0;
	int ticks = 0;
	bool new_year = false;

	std::vector<cell_data> cells;
	std::vector<int> seals;
	std::vector<int> bears;
	std::vector<int>* animals = nullptr;

	std::vector<int> seals_to_add;
	std::vector<int> bears_to_add;
	std::vector<int>* to_add = nullptr;

	struct {
		int ref_i = -1;
		std::vector<int>* refs = nullptr;
		ne::transform3f transform;
	} bb;

	psim();

	void update();
	void draw();

	void update_bears();
	void update_seals();

	void look(int index, look_info& info);
	bool try_move(int ref_i, int x, int y, uint32 terrain);
	bool move(int ref_i, look_info& info, uint32 terrain, int direction);
	bool try_birth(int cell_i, int x, int y, uint32 terrain);
	int can_breed(float chance, int amount);
	void breed(int cell_i, look_info& info, int amount, uint32 terrain);
	bool try_hunt(int cell_i, int x, int y);
	bool hunt(int cell_i, look_info& info);
	
	void kill_random(float chance);

	std::mt19937_64 rng;
	std::uniform_int_distribution<int> rng_direction;

};
