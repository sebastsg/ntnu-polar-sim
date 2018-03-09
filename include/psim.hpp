#pragma once

#include "assets.hpp"
#include <timer.hpp>
#include <graphics.hpp>

#define WORLD_SIZE    2048
#define TERRAIN_AT(I) textures.world.pixels[I]

#define BEAR     0xFFFFFF00
#define SEAL     0xFFFF0000
#define GROUND   0xFF338844
#define WATER    0xFFC8EBFF
#define ICE      0xFFEEEEEE

#define TICKS_PER_YEAR				500
#define RANDOM_DIRECTION			rng_direction(rng)
#define SWAP_AND_POP(V, I)			(V)[I] = (V).back(); (V).pop_back();
#define SET_ANIMAL(V)				animals = &V; to_add = &V##_to_add; stats.animals = &stats.##V;
#define MAX_TICKS_PER_DRAW			1
#define SIM_AUTO					1

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

#define LOG_ENABLED		0
#if LOG_ENABLED
#define LOG_DEATH_RANDOM  0
#define LOG_DEATH_EATEN   1
#define LOG_DEATH_STARVED 2
#define LOG_DEATH_AGED    3
struct sim_log {
	struct log_death {
		uint8 reason = 0;
		uint8 age = 0;
		float hunger = 0.0f;
		uint32 animal = 0;
		int tick = 0;
	};
	struct log_birth {
		uint32 animal = 0;
		int tick = 0;
	};
	std::vector<log_death> deaths;
	std::vector<log_birth> births;
	void death(uint8 reason, uint8 age, float hunger, uint32 animal, int tick) {
		deaths.push_back({ reason, age, hunger, animal, tick });
	}
	void birth(uint32 animal, int tick) {
		births.push_back({ animal, tick });
	}
};
#endif

// TODO: Make replay work properly.
#define RECORD_ENABLED				0
#define REPLAY_ENABLED				0
#if RECORD_ENABLED || REPLAY_ENABLED
#define LOAD_REPLAY					"stats/61885.re"
#define GROUND_INDEX 0
#define WATER_INDEX  1
#define BEAR_INDEX   2
#define SEAL_INDEX   3
#define PIXEL_INDEX_TO_PIXEL(INDEX) [&]{\
switch (INDEX) {\
case GROUND_INDEX: return GROUND;\
case WATER_INDEX: return WATER;\
case BEAR_INDEX: return BEAR;\
case SEAL_INDEX: return SEAL;\
}}();
#define PIXEL_TO_PIXEL_INDEX(PIXEL) [&]{\
switch (PIXEL) {\
case GROUND: return GROUND_INDEX;\
case WATER: return WATER_INDEX;\
case BEAR: return BEAR_INDEX;\
case SEAL: return SEAL_INDEX;\
}}();
struct replay_tick {
	struct cell {
		uint32 index = 0; // 22 bits
		uint32 pixel = 0; // 2 bits
	};
	std::vector<cell> cells;
};
struct replay_year {
	replay_tick ticks[TICKS_PER_YEAR];
	size_t current = 0;
	void push(int tick, uint32 index, uint32 pixel) {
		ticks[tick % TICKS_PER_YEAR].cells.push_back({ index, pixel });
	}
	replay_tick pop() {
		return ticks[current++];
	}
};
struct replay_simulation {
	std::vector<replay_year> years;
	void push(int year, int tick, uint32 index, uint32 pixel) {
		while (year >= years.size()) {
			years.push_back({});
		}
		years[year].push(tick, index, pixel);
	}
	replay_tick pop() {
		if (years.size() == 0) {
			return {};
		}
		replay_tick tick = years[0].pop();
		if (years[0].current >= TICKS_PER_YEAR) {
			years.erase(years.begin());
		}
		return tick;
	}
};
#endif

#define MODE_BALANCED				0
#define MODE_BEARS_DIE				1
#define MODE_BOTH_DIE				2
#define MODE_STRESS_TEST			3
#define SIM_MODE					MODE_BALANCED

#if SIM_MODE == MODE_BALANCED

#define INITIAL_BEAR_COUNT			1000
#define BEARS_DEAD_PER_YEAR			50
#define BEAR_MAX_AGE				30
#define BEAR_BREED_AGE				5
#define BEAR_BREED_PROBABILITY		0.3f
#define BEAR_HUNGER_HUNGRY			0.4f
#define BEAR_HUNGER_RATE			0.002f

#define INITIAL_SEAL_COUNT			100000
#define SEALS_DEAD_PER_YEAR			3000
#define SEAL_MAX_AGE				20
#define SEAL_BREED_AGE				3
#define SEAL_BREED_PROBABILITY		0.15f
#define SEAL_HUNGER_HUNGRY			0.2f
#define SEAL_HUNGER_RATE			0.0005f

#elif SIM_MODE == MODE_BEARS_DIE

#define INITIAL_BEAR_COUNT			1000
#define BEARS_DEAD_PER_YEAR			100
#define BEAR_MAX_AGE				30
#define BEAR_BREED_AGE				5
#define BEAR_BREED_PROBABILITY		0.2f
#define BEAR_HUNGER_HUNGRY			0.5f
#define BEAR_HUNGER_RATE			0.002f

#define INITIAL_SEAL_COUNT			100000
#define SEALS_DEAD_PER_YEAR			3000
#define SEAL_MAX_AGE				20
#define SEAL_BREED_AGE				3
#define SEAL_BREED_PROBABILITY		0.15f
#define SEAL_HUNGER_HUNGRY			0.2f
#define SEAL_HUNGER_RATE			0.0005f

#elif SIM_MODE == MODE_BOTH_DIE

#define INITIAL_BEAR_COUNT			1000
#define BEARS_DEAD_PER_YEAR			50
#define BEAR_MAX_AGE				30
#define BEAR_BREED_AGE				5
#define BEAR_BREED_PROBABILITY		0.5f
#define BEAR_HUNGER_HUNGRY			0.4f
#define BEAR_HUNGER_RATE			0.002f

#define INITIAL_SEAL_COUNT			100000
#define SEALS_DEAD_PER_YEAR			5000
#define SEAL_MAX_AGE				20
#define SEAL_BREED_AGE				3
#define SEAL_BREED_PROBABILITY		0.1f
#define SEAL_HUNGER_HUNGRY			0.2f
#define SEAL_HUNGER_RATE			0.0005f

#elif SIM_MODE == MODE_STRESS_TEST

#define INITIAL_BEAR_COUNT			10000
#define BEARS_DEAD_PER_YEAR			50
#define BEAR_MAX_AGE				30
#define BEAR_BREED_AGE				5
#define BEAR_BREED_PROBABILITY		0.3f
#define BEAR_HUNGER_HUNGRY			0.4f
#define BEAR_HUNGER_RATE			0.002f

#define INITIAL_SEAL_COUNT			1000000
#define SEALS_DEAD_PER_YEAR			3000
#define SEAL_MAX_AGE				20
#define SEAL_BREED_AGE				3
#define SEAL_BREED_PROBABILITY		0.15f
#define SEAL_HUNGER_HUNGRY			0.2f
#define SEAL_HUNGER_RATE			0.0005f

#endif

struct psim {

	ne::texture ani;

	struct {
		struct animal_stats {
			int dead_from_hunger = 0;
			int born = 0;
			int dead_from_age = 0;
			int dead_randomly = 0;

			// Related to kill():
			float kill_chance = 0.0f;
		} bears, seals;
		animal_stats* animals = nullptr;
		int seals_eaten_by_bears = 0;
	} stats;

#if LOG_ENABLED
	sim_log log;
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
		ne::font_text info;
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
	
	void kill(int per_year);

	std::mt19937_64 rng;
	std::uniform_int_distribution<int> rng_direction;

#if RECORD_ENABLED || REPLAY_ENABLED
	replay_simulation replay;
#endif

};
