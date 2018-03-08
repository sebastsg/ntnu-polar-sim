#version 130

uniform mat4 uni_ModelViewProjection;
uniform vec4 uni_Color;

in vec4 in_Position;
in vec4 in_Color;
in vec3 in_Tangent;
in vec3 in_Normal;
in vec2 in_TexCoords;

out vec4 ex_Color;
out vec2 ex_TexCoords;

void main() {
	gl_Position = uni_ModelViewProjection * in_Position;
	ex_Color = in_Color * uni_Color;
	ex_TexCoords = in_TexCoords;
}
