#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 Model = mat4(1.0);
uniform mat4 Projection = mat4(1.0);

void main()
{
	TexCoords = aTexCoords;

	gl_Position = Projection * Model * vec4(aPos.x, aPos.y, 0.0, 1.0);
}