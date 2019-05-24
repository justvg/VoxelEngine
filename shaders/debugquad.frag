#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D DebugTextureColors;
uniform sampler2D SSAO;

void main()
{
	vec3 Albedo = texture(DebugTextureColors, TexCoords).xyz;
	float Occlusion = texture(SSAO, TexCoords).r;
	vec3 Ambient = 0.3 * Albedo * Occlusion;

	Ambient = sqrt(Ambient);
	FragColor = vec4(Ambient, 1.0);
}