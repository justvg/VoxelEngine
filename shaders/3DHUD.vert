#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aColor;

out vec3 Color;

uniform mat4 View = mat4(1.0);
uniform mat4 Projection = mat4(1.0);

void main()
{
	Color = aColor;

	gl_Position = Projection * View * vec4(aPos, 1.0);
}
