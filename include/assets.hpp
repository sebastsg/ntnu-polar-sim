#pragma once

#include <asset.hpp>

void load_assets();
void destroy_assets();

class texture_assets : public ne::texture_group {
public:
	ne::texture blank;
	ne::texture world;
	void initialize();
};

class font_assets : public ne::font_group {
public:
	ne::font debug;
	void initialize();
};

class shader_assets : public ne::shader_group {
public:
	ne::shader basic;
	void initialize();
};

texture_assets& _textures();
#define textures _textures()

font_assets& _fonts();
#define fonts _fonts()

shader_assets& _shaders();
#define shaders _shaders()
