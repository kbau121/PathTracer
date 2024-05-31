#version 460

#define PI 3.14159265358979323

layout(location = 0) uniform uint indexCount;
layout(location = 1) uniform samplerBuffer indicesTex;

layout(location = 2) uniform samplerBuffer verticesTex;

layout(location = 3) uniform uvec2 resolution;
layout(location = 4) uniform vec3 eye;
layout(location = 5) uniform vec3 forward;
layout(location = 6) uniform vec3 up;
layout(location = 7) uniform vec3 right;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

const float FOVY = 19.5f * PI / 180.f;
Ray raycast()
{
    vec2 offset = vec2(0.5f);
    vec2 screenCoords = (gl_FragCoord.xy + offset) / resolution;
    screenCoords = screenCoords * 2.f - vec2(1.f);

    float aspectRatio = float(resolution.x) / resolution.y;
    vec3 ref = eye + forward;
    vec3 V = up * tan(FOVY * 0.5f);
    vec3 H = right * tan(FOVY * 0.5f) * aspectRatio;
    vec3 p = ref + H * screenCoords.x + V * screenCoords.y;

    return Ray(eye, normalize(p - eye));
}

bool intersect(Ray ray, out float t)
{
    t = 1.f / 0.f;
    for (int i = 0; i < indexCount / 3; ++i)
    {
        vec3 triangle = texelFetch(indicesTex, i).xyz;

        vec3 v0 = texelFetch(verticesTex, int(triangle.x)).xyz;
        vec3 v1 = texelFetch(verticesTex, int(triangle.y)).xyz;
        vec3 v2 = texelFetch(verticesTex, int(triangle.z)).xyz;

        vec3 e0 = v1 - v0;
        vec3 e1 = v2 - v0;

        vec3 n = normalize(cross(e0, e1));

        float sample_t = (dot(n, v0) - dot(n, ray.origin)) / dot(n, ray.direction);
        if (sample_t < 0.f || sample_t > t) continue;

        vec3 Q = ray.origin + sample_t * ray.direction;

        if (dot(cross(v1 - v0, Q - v0), n) < 0.f) continue;
        if (dot(cross(v2 - v1, Q - v1), n) < 0.f) continue;
        if (dot(cross(v0 - v2, Q - v2), n) < 0.f) continue;

        t = sample_t;
    }

    return !isinf(t);
}

void main()
{
    Ray ray = raycast();

    out_color = vec4(0.f, 0.f, 0.f, 1.f);
    float t;
    if (intersect(ray, t))
    {
        vec3 p = ray.origin + ray.direction * t;

        p = (p + vec3(1.f)) * 0.5f;

        out_color = vec4(p.x, p.y, p.z, 1.f);

        //out_color = vec4(1.f, 0.f, 0.f, 1.f);
    }
}