#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out VS_OUT 
{
	vec3 FragPosView;
	vec3 Color;
	vec3 Normal;	
} vs_out;


uniform mat4 Projection = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Model = mat4(1.0);

void main()
{
	vec4 FragPosView = View * Model * vec4(aPosition, 1.0);
	vs_out.FragPosView = vec3(FragPosView);
	vs_out.Normal = normalize(mat3(View * Model) * aNormal);
	vs_out.Color = aColor;

	gl_Position = Projection * FragPosView;
}


