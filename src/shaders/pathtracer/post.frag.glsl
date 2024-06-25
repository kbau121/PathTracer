#version 460

layout(location = 0) uniform sampler2D inTexture;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(texture(inTexture, texCoords).rgb, 1.f);
}