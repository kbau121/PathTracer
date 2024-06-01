#version 460

#define PI 3.14159265358979323

#define NUM_VERTEX_ATTRIBUTES 2
#define ATTRIBUTE_NORMAL 0
#define ATTRIBUTE_TEXTURE_COORDINATE 1

layout(location = 0) uniform uint indexCount;
layout(location = 1) uniform samplerBuffer indicesTex;

layout(location = 2) uniform samplerBuffer verticesTex;
layout(location = 3) uniform samplerBuffer vertexDataTex;

layout(location = 10) uniform uvec2 resolution;
layout(location = 11) uniform vec3 eye;
layout(location = 12) uniform vec3 forward;
layout(location = 13) uniform vec3 up;
layout(location = 14) uniform vec3 right;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 out_color;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

vec3 getVertexAttribute(int dataIndex, int vertexOffset, int vertexAttribute)
{
    return texelFetch(vertexDataTex, (dataIndex * 3 + vertexOffset) * NUM_VERTEX_ATTRIBUTES + vertexAttribute).xyz;
}

vec3 barycentricCoordinate(vec3 point, vec3 v0, vec3 v1, vec3 v2)
{
    vec3 e0 = v1 - v0;
    vec3 e1 = v2 - v0;
    vec3 eP = point - v0;

    float e0_e0 = dot(e0, e0);
    float e0_e1 = dot(e0, e1);
    float e1_e1 = dot(e1, e1);
    float e0_eP = dot(e0, eP);
    float e1_eP = dot(e1, eP);

    float invDenominator = 1.f / (e0_e0 * e1_e1 - e0_e1 * e0_e1);

    float v = (e1_e1 * e0_eP - e0_e1 * e1_eP) * invDenominator;
    float w = (e0_e0 * e1_eP - e0_e1 * e0_eP) * invDenominator;
    float u = 1.f - v - w;

    return vec3(u, v, w);
}

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

bool intersect(Ray ray, out float t, out vec3 nor)
{
    t = 1.f / 0.f;
    int ind = -1;
    for (int i = 0; i < indexCount; i ++)
    {
        vec4 triangle = texelFetch(indicesTex, i);

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
        ind = i;
    }

    if (ind == -1) return false;

    vec4 triangle = texelFetch(indicesTex, ind);
    int dataInd = int(triangle.w);

    vec3 p = ray.origin + ray.direction * t;

    vec3 v0 = texelFetch(verticesTex, int(triangle.x)).xyz;
    vec3 v1 = texelFetch(verticesTex, int(triangle.y)).xyz;
    vec3 v2 = texelFetch(verticesTex, int(triangle.z)).xyz;

    vec3 bary = barycentricCoordinate(p, v0, v1, v2);

    vec3 n1 = getVertexAttribute(dataInd, 0, ATTRIBUTE_NORMAL);
    vec3 n2 = getVertexAttribute(dataInd, 1, ATTRIBUTE_NORMAL);
    vec3 n3 = getVertexAttribute(dataInd, 2, ATTRIBUTE_NORMAL);
    nor = bary.x * n1 + bary.y * n2 + bary.z * n3;

    return true;
}

void main()
{
    Ray ray = raycast();

    out_color = vec4(0.f, 0.f, 0.f, 1.f);
    float t;
    vec3 nor;
    if (intersect(ray, t, nor))
    {
        vec3 p = ray.origin + ray.direction * t;

        out_color = vec4((nor + vec3(1.f)) * 0.5f, 1.f);
    }
}
