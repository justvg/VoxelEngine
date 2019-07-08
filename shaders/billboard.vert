#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform vec3 QuadCenterSimRegion;
uniform vec3 CameraRight;
uniform vec3 CameraUp = vec3(0.0, 1.0, 0.0);
uniform vec2 Scale = vec2(1.0, 1.0);

uniform mat4 View = mat4(1.0);
uniform mat4 Projection = mat4(1.0);

void main()
{
	TexCoords = aTexCoords;

	vec3 Pos = aPos * vec3(Scale, 1.0);
	Pos = Pos + vec3(-0.5*Scale.x, -0.5*Scale.y, 0.0);
	Pos = CameraRight * Pos.x + CameraUp * Pos.y;
	Pos = Pos + vec3(0.5*Scale.x, 0.5*Scale.y, 0.0);
	Pos = Pos + QuadCenterSimRegion;

	gl_Position = Projection * View * vec4(Pos, 1.0);
}