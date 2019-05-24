#version 330 core

out float BluredOcclusion;

in vec2 TexCoords;

uniform sampler2D Occlusions;

float Offsets[4] = float[](-1.5, -0.5, 0.5, 1.5);

void main()
{
	vec2 TexelSize = 1.0 / vec2(textureSize(Occlusions, 0));
	float Result = 0.0f;

	for(int Y = 0; Y < 4; Y++)
	{
		for(int X = 0; X < 4; X++)
		{
			vec2 Coords;
			Coords.x = TexCoords.x + Offsets[X] * TexelSize.x;
			Coords.y = TexCoords.y + Offsets[Y] * TexelSize.y;
			Result += texture(Occlusions, Coords).r;
		}
	}
	BluredOcclusion = Result / (4.0 * 4.0);
}