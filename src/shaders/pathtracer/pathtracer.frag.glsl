#version 460

layout(location = 0) uniform samplerBuffer verticesTex;
layout(location = 1) uniform samplerBuffer indicesTex;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

bool intersect(vec3 d)
{
    vec3 P = vec3(0.f);

    // TODO know how many triangles there are for maximum i
    for (int i = 0; i < 1; ++i)
    {
        vec3 triangle = texelFetch(indicesTex, i).xyz;
        //vec3 triangle = vec3(0, 1, 2);

        vec3 v0 = texelFetch(verticesTex, int(triangle.x)).xyz;
        vec3 v1 = texelFetch(verticesTex, int(triangle.y)).xyz;
        vec3 v2 = texelFetch(verticesTex, int(triangle.z)).xyz;

        vec3 e0 = v1 - v0;
        vec3 e1 = v2 - v0;

        vec3 n = normalize(cross(e0, e1));

        float t = (dot(n, v0) - dot(n, P)) / dot(n, d);
        vec3 Q = P + t * d;

        if (dot(cross(v1 - v0, Q - v0), n) < 0.f) continue;
        if (dot(cross(v2 - v1, Q - v1), n) < 0.f) continue;
        if (dot(cross(v0 - v2, Q - v2), n) < 0.f) continue;

        return true;
    }

    return false;
}

void main()
{
    vec2 screenCoords = (texCoords - vec2(0.5f)) * 2.f;
    vec3 dir = normalize(vec3(screenCoords, 0.25f));

    out_color = vec4(0.f, 0.f, 0.f, 1.f);
    if (intersect(dir))
    {
        out_color = vec4(1.f, 0.f, 0.f, 1.f);
    }
}