#version 330 core

layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in VS_OUT 
{
	vec3 FragPosView;
	vec3 Color;
	vec3 Normal;	
} fs_in;

void main()
{
	gPos = fs_in.FragPosView;
	gNormal = normalize(fs_in.Normal);
	gAlbedo = fs_in.Color;
}