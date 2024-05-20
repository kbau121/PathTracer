#version 460

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(texCoords.xy, 0.f, 1.f);
}