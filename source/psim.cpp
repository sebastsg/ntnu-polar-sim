#include "psim.hpp"
#include <engine.hpp>
#include <graphics.hpp>
#include <fstream>

psim::psim() {
	rng = std::mt19937_64(time(nullptr));
	rng_direction = std::uniform_int_distribution<int>(0, 7);

	ani.create();
	ani.pixels = new uint32[WORLD_SIZE * WORLD_SIZE];
	ani.size = WORLD_SIZE;
	memcpy(ani.pixels, textures.world.pixels, sizeof(uint32) * WORLD_SIZE * WORLD_SIZE);
	ani.render();

#if LOG_ENABLED
	log.reserve(1000000);
#endif
	cells.insert(cells.begin(), WORLD_SIZE * WORLD_SIZE, {});
	for (int i = 0; i < INITIAL_BEAR_COUNT; i++) {
		while (true) {
			int x = ne::random_int(WORLD_SIZE - 1);
			int y = ne::random_int(WORLD_SIZE - 1);
			int j = x + y * WORLD_SIZE;
			if (TERRAIN_AT(j) == WATER || cells[j].animal == BEAR) {
				continue;
			}
			cells[j] = { -1, (uint8)ne::random_int(BEAR_MAX_AGE), 0.0f, BEAR};
			bears.push_back(j);
			ani.pixels[j] = BEAR;
			break;
		}
	}
	for (int i = 0; i < INITIAL_SEAL_COUNT; i++) {
		while (true) {
			int x = ne::random_int(WORLD_SIZE - 1);
			int y = ne::random_int(WORLD_SIZE - 1);
			int j = x + y * WORLD_SIZE;
			if (TERRAIN_AT(j) != WATER || cells[j].animal != 0) {
				continue;
			}
			cells[j] = { -1, (uint8)ne::random_int(SEAL_MAX_AGE), 0.0f, SEAL};
			seals.push_back(j);
			ani.pixels[j] = SEAL;
			break;
		}
	}
	ne::listen([&](ne::keyboard_key_message key) {
		if (key.is_pressed) {
			if (key.key == KEY_B) {
				bb.refs = &bears;
				bb.ref_i = ne::random_int(bears.size() - 1);
			} else if (key.key == KEY_N) {
				bb.refs = &seals;
				bb.ref_i = ne::random_int(seals.size() - 1);
			}
		}
	});
#if LOG_ENABLED
	ne::listen([&](ne::keyboard_key_message key) {
		if (key.is_pressed && key.key == KEY_0) {
			std::ofstream of(STRING(time(nullptr) << ".csv"));
			of << "Age;Hunger;Animal;TimeOfDeath;\n";
			for (auto& i : log) {
				of << (int)i.age_dead << ";";
				of << i.hunger_dead << ";";
				if (i.animal == BEAR) {
					of << "Bear;";
				} else if (i.animal == SEAL) {
					of << "Seal;";
				}
				of << i.time_of_death << ";\n";
			}
		}
	});
#endif
}

void psim::look(int index, look_info& info) {
	info.coord = { index % WORLD_SIZE, index / WORLD_SIZE };
	info.left_x = info.coord.x - 1;
	info.top_y = info.coord.y - 1;
	info.right_x = info.coord.x + 1;
	info.bottom_y = info.coord.y + 1;
	if (info.left_x < 0) {
		info.left_x = WORLD_SIZE - 1;
	} else if (info.right_x > WORLD_SIZE - 1) {
		info.right_x = 0;
	}
	if (info.top_y < 0) {
		info.top_y = WORLD_SIZE - 1;
	} else if (info.bottom_y > WORLD_SIZE - 1) {
		info.bottom_y = 0;
	}
}

bool psim::try_move(int ref_i, int x, int y, uint32 terrain) {
	const int move_index = x + y * WORLD_SIZE;
	if (TERRAIN_AT(move_index) == terrain && cells[move_index].animal == 0) {
		int& cell_i = (*animals)[ref_i];
		ani.pixels[cell_i] = TERRAIN_AT(cell_i);
		std::swap(cells[move_index], cells[cell_i]);
		ani.pixels[move_index] = cells[move_index].animal;
		cell_i = move_index;
		return true;
	}
	return false;
}

bool psim::move(int ref_i, look_info& info, uint32 terrain, int direction) {
	switch (direction) {
	case 0: return try_move(ref_i, info.left_x, info.top_y, terrain);
	case 1: return try_move(ref_i, info.coord.x, info.top_y, terrain);
	case 2: return try_move(ref_i, info.right_x, info.top_y, terrain);
	case 3: return try_move(ref_i, info.left_x, info.coord.y, terrain);
	case 4: return try_move(ref_i, info.right_x, info.coord.y, terrain);
	case 5: return try_move(ref_i, info.left_x, info.bottom_y, terrain);
	case 6: return try_move(ref_i, info.coord.x, info.bottom_y, terrain);
	case 7: return try_move(ref_i, info.right_x, info.bottom_y, terrain);
	default: return false;
	}
}

bool psim::try_birth(int cell_i, int x, int y, uint32 terrain) {
	const int birth_index = x + y * WORLD_SIZE;
	if (TERRAIN_AT(birth_index) == terrain && cells[birth_index].animal == 0) {
		cells[birth_index] = { -1, 0, 0.0f, cells[cell_i].animal/*, -1*/ };
		ani.pixels[birth_index] = cells[cell_i].animal;
		to_add->push_back(birth_index);
		stats.animals->born++;
		// TODO: log
		return true;
	}
	return false;
}

int psim::can_breed(float chance, int amount) {
	if (!ne::random_chance(chance)) {
		return 0;
	}
	return 1 + ne::random_int(amount - 1);
}

void psim::breed(int cell_i, look_info& info, int amount, uint32 terrain) {
	for (int i = 0; i < amount; i++) {
		if (try_birth(cell_i, info.left_x, info.top_y, terrain)) continue;
		if (try_birth(cell_i, info.coord.x, info.top_y, terrain)) continue;
		if (try_birth(cell_i, info.right_x, info.top_y, terrain)) continue;
		if (try_birth(cell_i, info.left_x, info.coord.y, terrain)) continue;
		if (try_birth(cell_i, info.right_x, info.coord.y, terrain)) continue;
		if (try_birth(cell_i, info.left_x, info.bottom_y, terrain)) continue;
		if (try_birth(cell_i, info.coord.x, info.bottom_y, terrain)) continue;
		if (try_birth(cell_i, info.right_x, info.bottom_y, terrain)) continue;
	}
}

bool psim::try_hunt(int cell_i, int x, int y) {
	const int hunt_index = x + y * WORLD_SIZE;
	if (cells[hunt_index].animal == SEAL) {
		cells[cell_i].hunger = 0.0f;
		for (size_t i = 0; i < seals.size(); i++) {
			if (seals[i] == hunt_index) {
				SWAP_AND_POP(seals, i);
				break;
			}
		}
		cells[hunt_index] = {};
		ani.pixels[hunt_index] = TERRAIN_AT(hunt_index);
		stats.seals_eaten_by_bears++;
		// TODO: log
		return true;
	}
	return false;
}

bool psim::hunt(int cell_i, look_info& info) {
	if (try_hunt(cell_i, info.left_x, info.top_y)) return true;
	if (try_hunt(cell_i, info.coord.x, info.top_y)) return true;
	if (try_hunt(cell_i, info.right_x, info.top_y)) return true;
	if (try_hunt(cell_i, info.left_x, info.coord.y)) return true;
	if (try_hunt(cell_i, info.right_x, info.coord.y)) return true;
	if (try_hunt(cell_i, info.left_x, info.bottom_y)) return true;
	if (try_hunt(cell_i, info.coord.x, info.bottom_y)) return true;
	if (try_hunt(cell_i, info.right_x, info.bottom_y)) return true;
	return false;
}

void psim::kill_random(float chance) {
	if (!ne::random_chance(chance)) {
		return;
	}
	for (size_t i = 100; i < animals->size(); i++) {
#if LOG_ENABLED
		log.push_back({ i.second->age, i.second->hunger, i.second->animal, ticks });
#endif
		int cell_i = (*animals)[i];
		cells[cell_i] = {};
		ani.pixels[cell_i] = TERRAIN_AT(cell_i);
		SWAP_AND_POP(*animals, i);
		stats.animals->dead_randomly++;
		break;
	}
}

void psim::update_bears() {
	look_info info;
	SET_ANIMAL(bears);
	kill_random(RANDOM_BEAR_DEATH_CHANCE);
	for (int ref_i = 0; ref_i < bears.size(); ref_i++) {
		int& cell_i = bears[ref_i];
		auto& bear = cells[cell_i];
		look(cell_i, info);
		bear.hunger += HUNGER_TO_DIE_IN_A_YEAR * 0.2f;
		if (bear.hunger > 1.0f) {
#if LOG_ENABLED
			log.push_back({ bear.age, bear.hunger, bear.animal, ticks });
#endif
			stats.bears.dead_from_hunger++;
			bear = {};
			ani.pixels[cell_i] = TERRAIN_AT(cell_i);
			SWAP_AND_POP(bears, ref_i);
			ref_i--;
			continue;
		}
		if (bear.hunger > 0.2f) {
			hunt(cell_i, info);
			if (TERRAIN_AT(cell_i) == GROUND) {
				if (move(ref_i, info, WATER, RANDOM_DIRECTION)) {
					continue;
				}
			}
			int direction = RANDOM_DIRECTION;
			if (move(ref_i, info, GROUND, direction)) {
				continue;
			}
		}
		if (move(ref_i, info, GROUND, RANDOM_DIRECTION)) {
			continue;
		}
	}
	if (new_year) {
		for (int ref_i = 0; ref_i < bears.size(); ref_i++) {
			int& cell_i = bears[ref_i];
			auto& bear = cells[cell_i];
			if (++bear.age >= BEAR_BREED_AGE) {
				if (bear.age > BEAR_MAX_AGE) {
					cells[cell_i] = {};
					ani.pixels[cell_i] = TERRAIN_AT(cell_i);
					SWAP_AND_POP(bears, ref_i);
					ref_i--;
					stats.bears.dead_from_age++;
					continue;
				}
				int amount = can_breed(BEAR_BREED_PROBABILITY, 2);
				if (amount > 0) {
					look(cell_i, info);
					breed(cell_i, info, amount, GROUND);
				}
			}
		}
	}
	for (auto& i : bears_to_add) {
		bears.push_back(i);
	}
	bears_to_add.clear();
}

void psim::update_seals() {
	look_info info;
	SET_ANIMAL(seals);
	kill_random(RANDOM_SEAL_DEATH_CHANCE);
	for (int ref_i = 0; ref_i < seals.size(); ref_i++) {
		int& cell_i = seals[ref_i];
		auto& seal = cells[cell_i];
		look(cell_i, info);
		seal.hunger += HUNGER_TO_DIE_IN_A_YEAR * 0.1f;
		if (TERRAIN_AT(cell_i) == GROUND) {
			if (seal.hunger > 0.1f) {
				seal.hunger -= 0.01f;
			} else {
				if (move(ref_i, info, WATER, RANDOM_DIRECTION)) {
					seal.hunger = 0.0f;
					continue;
				} else {
					seal.hunger += 0.1f;
				}
			}
			continue;
		}
		if (seal.hunger > 1.0f) {
#if LOG_ENABLED
			log.push_back({ seal.age, seal.hunger, seal.animal, ticks });
#endif
			stats.seals.dead_from_hunger++;
			seal = {};
			ani.pixels[cell_i] = TERRAIN_AT(cell_i);
			SWAP_AND_POP(seals, ref_i);
			ref_i--;
			continue;
		}
		if (seal.hunger > 0.3f) {
			if (seal.direction == -1) {
				seal.direction = RANDOM_DIRECTION;
			}
			if (move(ref_i, info, GROUND, seal.direction)) {
				seal.direction = -1;
				continue;
			}
			if (move(ref_i, info, WATER, seal.direction)) {
				if (info.coord.x % (seal.age + 1) == 0 || info.coord.y % (seal.age + 1) == 0) {
					seal.direction = RANDOM_DIRECTION;
				}
				continue;
			} else {
				seal.direction = RANDOM_DIRECTION;
			}
		} else {
			move(ref_i, info, WATER, RANDOM_DIRECTION);
			continue;
		}
	}
	if (new_year) {
		for (int ref_i = 0; ref_i < seals.size(); ref_i++) {
			int cell_i = seals[ref_i];
			auto& seal = cells[cell_i];
			if (++seal.age >= SEAL_BREED_AGE) {
				if (seal.age > SEAL_MAX_AGE) {
					cells[cell_i] = {};
					ani.pixels[cell_i] = TERRAIN_AT(cell_i);
					SWAP_AND_POP(seals, ref_i);
					ref_i--;
					stats.seals.dead_from_age++;
					continue;
				}
				if (TERRAIN_AT(cell_i) == GROUND) {
					int amount = can_breed(SEAL_BREED_PROBABILITY, 2);
					if (amount > 0) {
						look(cell_i, info);
						breed(cell_i, info, amount, WATER);
					}
				}
			}
		}
	}
	for (auto& i : seals_to_add) {
		seals.push_back(i);
	}
	seals_to_add.clear();
}

void psim::update() {
	if (!ne::is_key_down(KEY_SPACE)) {
		return;
	}
	int old_year = year;
	year = ticks / TICKS_PER_YEAR;
	new_year = (old_year != year);
	update_bears();
	update_seals();
	ticks++;
}

void psim::draw() {
	ne::shader::set_color(1.0f);
	ne::transform3f transform;
	transform.scale.xy = ani.size.to<float>();
	ne::shader::set_transform(&transform);
	ani.bind();
	ani.refresh();
	ne::drawing_shape::bound()->draw();
	if (bb.refs && bb.ref_i != -1) {
		if (bb.ref_i >= bb.refs->size() || bb.ref_i < 0) {
			bb.ref_i = -1;
			bb.refs = nullptr;
			return;
		}
		int cell_i = (*bb.refs)[bb.ref_i];
		if (cells[cell_i].animal == 0) {
			bb.ref_i = -1;
			bb.refs = nullptr;
			return;
		}
		textures.blank.bind();
		ne::shader::set_color({ 1.0f, 0.0f, 1.0f, 1.0f });
		bb.transform.position.x = (float)(cell_i % WORLD_SIZE);
		bb.transform.position.y = (float)(cell_i / WORLD_SIZE);
		bb.transform.scale.xy = 1.0f;
		ne::shader::set_transform(&bb.transform);
		ne::drawing_shape::bound()->draw();
		ne::ortho_camera::bound()->target = &bb.transform;
	} else {
		ne::ortho_camera::bound()->target = nullptr;
	}
}
