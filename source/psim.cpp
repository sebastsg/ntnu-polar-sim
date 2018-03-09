#include "psim.hpp"
#include <engine.hpp>
#include <memory.hpp>
#include <platform.hpp>
#include <fstream>
#include <bitset>

psim::psim() {
	rng = std::mt19937_64(time(nullptr));
	rng_direction = std::uniform_int_distribution<int>(0, 7);
	
	if (!std::experimental::filesystem::is_directory("stats")) {
		std::experimental::filesystem::create_directory("stats");
	}

#if RECORD_ENABLED || REPLAY_ENABLED
	if (WORLD_SIZE > 2048) {
		NE_ERROR("Cannot have world size over 2048 when recording is enabled.");
		std::exit(1);
	}
#endif

	ani.create();
	ani.pixels = new uint32[WORLD_SIZE * WORLD_SIZE];
	ani.size = WORLD_SIZE;
	memcpy(ani.pixels, textures.world.pixels, sizeof(uint32) * WORLD_SIZE * WORLD_SIZE);
	ani.render();

#if !REPLAY_ENABLED
	cells.insert(cells.begin(), WORLD_SIZE * WORLD_SIZE, {});
	for (int i = 0; i < INITIAL_BEAR_COUNT; i++) {
		while (true) {
			int x = ne::random_int(WORLD_SIZE - 1);
			int y = ne::random_int(WORLD_SIZE - 1);
			int j = x + y * WORLD_SIZE;
			if (TERRAIN_AT(j) == WATER || cells[j].animal == BEAR) {
				continue;
			}
			cells[j] = { -1, (uint8)ne::random_int(BEAR_MAX_AGE), ne::random_float(0.0f, 0.5f), BEAR};
			bears.push_back(j);
			ani.pixels[j] = BEAR;
#if RECORD_ENABLED
			replay.push(year, ticks, j, ani.pixels[j]);
#endif
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
			cells[j] = { -1, (uint8)ne::random_int(SEAL_MAX_AGE), ne::random_float(0.0f, 0.5f), SEAL};
			seals.push_back(j);
			ani.pixels[j] = SEAL;
#if RECORD_ENABLED
			replay.push(year, ticks, j, ani.pixels[j]);
#endif
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
			const time_t t = time(nullptr) - 1520561000;
			std::ofstream of(STRING("stats/deaths_" << t << ".csv"));
			of << "Reason;Age;Hunger;Animal;Tick;\n";
			for (auto& i : log.deaths) {
				switch (i.reason) {
				case LOG_DEATH_RANDOM: of << "Random;"; break;
				case LOG_DEATH_EATEN: of << "Eaten;"; break;
				case LOG_DEATH_STARVED: of << "Starved;"; break;
				case LOG_DEATH_AGED: of << "Aged;"; break;
				default: of << "Unknown;"; break;
				}
				of << (int)i.age << ";";
				of << i.hunger << ";";
				if (i.animal == BEAR) {
					of << "Bear;";
				} else if (i.animal == SEAL) {
					of << "Seal;";
				} else {
					of << "Unknown;";
				}
				of << i.tick<< ";\n";
			}
			of = std::ofstream(STRING("stats/births_" << t << ".csv"));
			of << "Animal;Tick;\n";
			for (auto& i : log.births) {
				if (i.animal == BEAR) {
					of << "Bear;";
				} else if (i.animal == SEAL) {
					of << "Seal;";
				} else {
					of << "Unknown";
				}
				of << i.tick << ";\n";
			}
		}
	});
#endif
#endif
	ne::listen([&](ne::keyboard_key_message key) {
		if (!key.is_pressed) {
			return;
		}
		if (key.key == KEY_O) {
#if RECORD_ENABLED
			ne::memory_buffer buffer;
			buffer.write_uint32(replay.years.size());
			for (auto& i : replay.years) {
				for (auto& j : i.ticks) {
					buffer.write_uint32(j.cells.size());
					for (auto& k : j.cells) {
						// TODO: Remove duplicates that are overwritten
						uint32 data = (k.index | (k.pixel << 2));
						buffer.write_uint8(data >> 16);
						buffer.write_uint8(data >> 8);
						buffer.write_uint8(data);
					}
				}
			}
			const time_t t = time(nullptr) - 1520561000;
			ne::write_file(STRING("stats/" << t << ".re"), buffer.begin, buffer.write_index());
			buffer.free();
#endif
		} else if (key.key == KEY_P) {
#if REPLAY_ENABLED
			if (!ne::file_exists(LOAD_REPLAY)) {
				return;
			}
			ne::memory_buffer buffer;
			ne::read_file(LOAD_REPLAY, &buffer);
			for (size_t i = 0; i < WORLD_SIZE * WORLD_SIZE; i++) {
				replay.push(year, ticks, i, ani.pixels[i]);
			}
			uint32 year_count = buffer.read_uint32();
			for (uint32 i = 0; i < year_count; i++) {
				replay.years.push_back({});
				for (uint32 j = 0; j < TICKS_PER_YEAR; j++) {
					uint32 cell_count = buffer.read_uint32();
					for (uint32 k = 0; k < cell_count; k++) {
						uint8 b0 = buffer.read_uint8();
						uint8 b1 = buffer.read_uint8();
						uint8 b2 = buffer.read_uint8();
						uint32_t data = (b0 << 16) | (b1 << 8) | b2;
						replay.years[i].ticks[j].cells.push_back({ data & ((1 << 2) - 1), data >> 2 });
					}
				}
			}
			buffer.free();
#endif
		}
	});
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
#if RECORD_ENABLED
		replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
		replay.push(year, ticks, move_index, ani.pixels[move_index]);
#endif
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
		cells[birth_index] = { -1, 0, 0.0f, cells[cell_i].animal };
		ani.pixels[birth_index] = cells[cell_i].animal;
#if LOG_ENABLED
		log.birth(cells[birth_index].animal, ticks);
#endif
#if RECORD_ENABLED
		replay.push(year, ticks, birth_index, ani.pixels[birth_index]);
#endif
		to_add->push_back(birth_index);
		stats.animals->born++;
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
		cells[cell_i].hunger /= 2.0f;
		for (size_t i = 0; i < seals.size(); i++) {
			if (seals[i] == hunt_index) {
				SWAP_AND_POP(seals, i);
				break;
			}
		}
#if LOG_ENABLED
		log.death(LOG_DEATH_EATEN, cells[hunt_index].age, cells[hunt_index].hunger, cells[hunt_index].animal, ticks);
#endif
		cells[hunt_index] = {};
		ani.pixels[hunt_index] = TERRAIN_AT(hunt_index);
#if RECORD_ENABLED
		replay.push(year, ticks, hunt_index, ani.pixels[hunt_index]);
#endif
		stats.seals_eaten_by_bears++;
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

void psim::kill(int per_year) {
	const float per_tick = (float)per_year / (float)TICKS_PER_YEAR;
	stats.animals->kill_chance += per_tick;
	for (size_t i = 0; i < animals->size() && stats.animals->kill_chance >= 1.0f; i++) {
		int cell_i = (*animals)[i];
#if LOG_ENABLED
		log.death(LOG_DEATH_RANDOM, cells[cell_i].age, cells[cell_i].hunger, cells[cell_i].animal, ticks);
#endif
		cells[cell_i] = {};
		ani.pixels[cell_i] = TERRAIN_AT(cell_i);
#if RECORD_ENABLED
		replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
#endif
		SWAP_AND_POP(*animals, i);
		stats.animals->dead_randomly++;
		stats.animals->kill_chance--;
	}
}

void psim::update_bears() {
	look_info info;
	SET_ANIMAL(bears);
	kill(BEARS_DEAD_PER_YEAR);
	for (int ref_i = 0; ref_i < bears.size(); ref_i++) {
		int& cell_i = bears[ref_i];
		auto& bear = cells[cell_i];
		look(cell_i, info);
		bear.hunger += BEAR_HUNGER_RATE;
		if (bear.hunger > 1.0f) {
#if LOG_ENABLED
			log.death(LOG_DEATH_STARVED, bear.age, bear.hunger, bear.animal, ticks);
#endif
			stats.bears.dead_from_hunger++;
			bear = {};
			ani.pixels[cell_i] = TERRAIN_AT(cell_i);
#if RECORD_ENABLED
			replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
#endif
			SWAP_AND_POP(bears, ref_i);
			ref_i--;
			continue;
		}
		hunt(cell_i, info);
		if (bear.direction == -1) {
			bear.direction = RANDOM_DIRECTION;
		}
		if (bear.hunger > BEAR_HUNGER_HUNGRY) {
			if (TERRAIN_AT(cell_i) == GROUND) {
				if (move(ref_i, info, WATER, bear.direction)) {
					bear.direction = -1;
					continue;
				}
			}
			if (move(ref_i, info, GROUND, bear.direction)) {
				continue;
			}
		}
		if (!move(ref_i, info, GROUND, bear.direction)) {
			bear.direction = -1;
		}
	}
	if (new_year) {
		for (int ref_i = 0; ref_i < bears.size(); ref_i++) {
			int& cell_i = bears[ref_i];
			auto& bear = cells[cell_i];
			if (++bear.age >= BEAR_BREED_AGE) {
				if (bear.age > BEAR_MAX_AGE) {
#if LOG_ENABLED
					log.death(LOG_DEATH_AGED, bear.age, bear.hunger, bear.animal, ticks);
#endif
					cells[cell_i] = {};
					ani.pixels[cell_i] = TERRAIN_AT(cell_i);
#if RECORD_ENABLED
					replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
#endif
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
	kill(SEALS_DEAD_PER_YEAR);
	for (int ref_i = 0; ref_i < seals.size(); ref_i++) {
		int& cell_i = seals[ref_i];
		auto& seal = cells[cell_i];
		look(cell_i, info);
		seal.hunger += SEAL_HUNGER_RATE;
		if (new_year) {
			seal.age++;
		}
		if (TERRAIN_AT(cell_i) == GROUND) {
			if (seal.hunger > 0.1f) {
				seal.hunger -= 0.01f;
			} else {
				if (seal.age >= SEAL_BREED_AGE) {
					if (seal.age > SEAL_MAX_AGE) {
#if LOG_ENABLED
						log.death(LOG_DEATH_AGED, seal.age, seal.hunger, seal.animal, ticks);
#endif
						cells[cell_i] = {};
						ani.pixels[cell_i] = TERRAIN_AT(cell_i);
#if RECORD_ENABLED
						replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
#endif
						SWAP_AND_POP(seals, ref_i);
						ref_i--;
						stats.seals.dead_from_age++;
						continue;
					}
					int amount = can_breed(SEAL_BREED_PROBABILITY, 2);
					if (amount > 0) {
						breed(cell_i, info, amount, WATER);
					}
				}
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
			log.death(LOG_DEATH_STARVED, seal.age, seal.hunger, seal.animal, ticks);
#endif
			stats.seals.dead_from_hunger++;
			seal = {};
			ani.pixels[cell_i] = TERRAIN_AT(cell_i);
#if RECORD_ENABLED
			replay.push(year, ticks, cell_i, ani.pixels[cell_i]);
#endif
			SWAP_AND_POP(seals, ref_i);
			ref_i--;
			continue;
		}
		if (seal.hunger > SEAL_HUNGER_HUNGRY) {
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
	for (auto& i : seals_to_add) {
		seals.push_back(i);
	}
	seals_to_add.clear();
}

void psim::update() {
#if !SIM_AUTO
	if (!ne::is_key_down(KEY_SPACE)) {
		return;
	}
#endif
	int old_year = year;
	year = ticks / TICKS_PER_YEAR;
	new_year = (old_year != year);
#if REPLAY_ENABLED
	replay_tick current_tick = replay.pop();
	if (current_tick.cells.size() == 0) {
		NE_WARNING_LIMIT("No more ticks in recording.", 1);
	}
	for (auto& i : current_tick.cells) {
		ani.pixels[i.index] = PIXEL_INDEX_TO_PIXEL(i.pixel);
	}
#else
	update_bears();
	update_seals();
#endif
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
#if !REPLAY_ENABLED
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
		bb.info.font = &fonts.debug;
		bool rendered = bb.info.render(STRING(
			"Hunger: " << (int)(cells[cell_i].hunger * 100.0f) <<
			"\nAge: " << (int)cells[cell_i].age
		));
		bb.info.transform.position = bb.transform.position;
		if (rendered) {
			bb.info.transform.scale.x /= 4.0f;
			bb.info.transform.scale.y /= 4.0f;
		}
		bb.info.transform.position.x -= bb.info.transform.scale.width / 2.0f;
		bb.info.transform.position.y -= bb.info.transform.scale.height;
		ne::shader::set_color({ 0.0f, 0.0f, 0.0f, 1.0f });
		bb.info.draw();
		bb.info.transform.position.x -= 0.25f;
		bb.info.transform.position.y -= 0.25f;
		ne::shader::set_color({ 1.0f, 0.0f, 1.0f, 1.0f });
		bb.info.draw();
	} else {
		ne::ortho_camera::bound()->target = nullptr;
	}
#endif
}
