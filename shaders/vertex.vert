#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aOcclusion;

out vec3 Color;
out vec3 Normal;
out vec3 LightDir;
out vec3 FragPosView;
out float Occlusion;

vec3 DirectionalLightDir = vec3(0.5, -0.3, -0.4);

uniform mat4 Projection = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Model = mat4(1.0);

void main()
{
	vec4 Temp = View * Model * vec4(aPosition, 1.0);
	FragPosView = vec3(Temp);
	Normal = mat3(View * Model) * aNormal;
	LightDir = mat3(View * Model) * DirectionalLightDir;
	Color = aColor;
	Occlusion = aOcclusion;

	gl_Position = Projection * Temp;
}


