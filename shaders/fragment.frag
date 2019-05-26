#version 330 core

out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 LightDir;
in float Occlusion;
in vec3 FragPosView;

void main()
{
	vec3 Ambient = 0.2 * Color * Occlusion;

	vec3 LightDir = normalize(-LightDir);
	vec3 Diffuse = max(dot(LightDir, Normal), 0.0) * Color;

	vec3 ViewDir = normalize(-FragPosView);
	vec3 Reflection = reflect(-LightDir, Normal);
	vec3 Specular = pow(max(dot(ViewDir, Reflection), 0.0), 5) * Color; 

	vec3 FinalColor = Ambient + Diffuse + Specular;
	FinalColor = sqrt(FinalColor);
	FragColor = vec4(FinalColor, 1.0);
}