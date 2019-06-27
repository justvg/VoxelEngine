#version 330 core

out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 inLightDir;
in float Occlusion;
in vec3 FragPosView;

vec3 Fog(vec3 SourceColor, vec3 FogColor, float Distance, float FogDensity)
{
	const float e = 2.71828182845904523536028747135266249;
	float f = pow(e, -pow((Distance*FogDensity), 2));
	vec3 Result = mix(FogColor, SourceColor, f);
	return (Result);
}

void main()
{
	//vec3 Ambient = 0.2 * Color * Occlusion;
	vec3 Ambient = 0.2 * Color;

	vec3 LightDir = normalize(-inLightDir);
	vec3 Diffuse = max(dot(LightDir, Normal), 0.0) * Color;

	vec3 ViewDir = normalize(-FragPosView);
	vec3 Reflection = reflect(-LightDir, Normal);
	vec3 Specular = pow(max(dot(ViewDir, Reflection), 0.0), 5) * Color; 

	vec3 FinalColor = Ambient + Diffuse + Specular;
	FinalColor = Fog(FinalColor, vec3(0.2549, 0.4117, 1.0), FragPosView.z, 0.01);
	FinalColor = sqrt(FinalColor);
	FragColor = vec4(FinalColor, 1.0);
}