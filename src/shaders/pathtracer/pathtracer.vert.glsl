#version 460

layout(location = 0) out vec2 texCoords;

const vec2 vertices[3] = vec2[3](vec2(-1.f, -1.f), vec2(3.f, -1.f), vec2(-1.f, 3.f));

void main()
{
    gl_Position = vec4(vertices[gl_VertexID], 0.f, 1.f);
    texCoords = 0.5f * gl_Position.xy + vec2(0.5f);
}