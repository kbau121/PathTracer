#version 460

layout(location = 0) uniform sampler2D inTexture;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

void main()
{
	vec3 color = texture(inTexture, texCoords).rgb;
	// Reinhard operator
	color = color / (vec3(1.f) + color);
	// Gamma correction
	color = pow(color, vec3(1.f / 2.2f));

	out_color = vec4(color, 1.f);
}