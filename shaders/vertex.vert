#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 Color;
out vec3 Normal;
out vec3 LightDir;

vec3 DirectionalLightDir = vec3(0.5, -0.3, -0.4);

uniform mat4 Projection = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Model = mat4(1.0);

void main()
{
	Normal = mat3(View) * aNormal;
	LightDir = mat3(View) * DirectionalLightDir;
	Color = aColor;

	gl_Position = Projection * View * Model * vec4(aPosition, 1.0);
}


