#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Color;
out vec3 Normal;
out vec3 FragPosView;
out vec3 inLightDir;

uniform vec3 inColor;
uniform mat4 Model = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Projection = mat4(1.0);

vec3 DirectionalLightDir = vec3(0.5, -0.3, -0.4);

void main()
{
	Color = inColor;
	vec4 Temp = View * Model * vec4(aPos, 1.0);
	FragPosView = vec3(Temp);
	Normal = normalize(mat3(View * Model) * aNormal);
	inLightDir = normalize(mat3(View) * DirectionalLightDir);
	gl_Position = Projection * Temp;
}