#include "assets.hpp"

#include <engine.hpp>

struct asset_container {
	texture_assets _textures;
	font_assets _fonts;
	shader_assets _shaders;
};

static asset_container* _assets = nullptr;

void load_assets() {
	if (_assets) {
		return;
	}
	_assets = new asset_container();
	_assets->_textures.initialize();
	_assets->_fonts.initialize();
	_assets->_shaders.initialize();
	_assets->_textures.load_all();
	_assets->_fonts.load_all();
	_assets->_shaders.load_all();
	_assets->_textures.process_some(1000);
	_assets->_fonts.process_some(1000);
	_assets->_shaders.process_some(1000);
}

void destroy_assets() {
	if (!_assets) {
		return;
	}
	delete _assets;
	_assets = nullptr;
}

texture_assets& _textures() {
	return _assets->_textures;
}

font_assets& _fonts() {
	return _assets->_fonts;
}

shader_assets& _shaders() {
	return _assets->_shaders;
}

void texture_assets::initialize() {
	root("assets/textures");
	load({ &blank, "blank.png" });
	load({ &world, "world.png", 1, TEXTURE_PIXELS_IN_MEMORY });
	spawn_thread();
	finish();
}

void font_assets::initialize() {
	root("assets/fonts");
	load({ &debug, "troika.otf", 16, false });
}

void shader_assets::initialize() {
	root("assets/shaders");
	load({ &basic, "basic" });
}
