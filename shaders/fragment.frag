#version 330 core

out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 LightDir;

void main()
{
	vec3 LightDir = normalize(-LightDir);
	vec3 Diffuse = max(dot(LightDir, Normal), 0.0) * Color;

	Diffuse = sqrt(Diffuse);
	FragColor = vec4(Diffuse, 1.0);
}