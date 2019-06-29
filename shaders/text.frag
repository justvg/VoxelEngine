#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D Texture;
uniform vec3 TextColor;

void main()
{
	vec4 Sampled = vec4(1.0, 1.0, 1.0, texture(Texture, TexCoords).r);
	FragColor = vec4(TextColor, 1.0) * Sampled;
}
