#version 460

#define PI     3.14159265358979323
#define INV_PI 0.31830988618379067

// Vertex attributes
#define NUM_VERTEX_ATTRIBUTES 2
#define ATTRIBUTE_NORMAL 0
#define ATTRIBUTE_TEXTURE_COORDINATE 1

// Material attributes
#define ALBEDO 0
#define ROUGHNESS 3
#define METALLIC 4
#define MATERIAL_SIZE (1 + METALLIC)

// Object types
#define GEOMETRY 0
#define LIGHT 1

layout(location = 0) uniform uint indexCount;
layout(location = 1) uniform samplerBuffer indicesTex;

layout(location = 2) uniform samplerBuffer verticesTex;
layout(location = 3) uniform samplerBuffer vertexDataTex;

layout(location = 4) uniform sampler2D accumTexture;
layout(location = 5) uniform uint iterationCount;

layout(location = 6) uniform samplerBuffer lightTex;
layout(location = 7) uniform uvec4 lightCount;

layout(location = 8) uniform samplerBuffer materialTex;
layout(location = 9) uniform usamplerBuffer materialMapTex;

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

struct Intersection
{
    float t;
    vec3 normal;
    vec3 radiance;

    int type;
    int index;
};

struct Light
{
    vec3 radiance;
    mat4 transform;
    mat4 invTransform;
};

struct Material
{
    vec3 albedo;
    float roughness;
    float metallic;
};

vec3 getVertexAttribute(int dataIndex, int vertexOffset, int vertexAttribute)
{
    return texelFetch(vertexDataTex, (dataIndex * 3 + vertexOffset) * NUM_VERTEX_ATTRIBUTES + vertexAttribute).xyz;
}

Light getLight(int index)
{
    vec3 radiance = texelFetch(lightTex, index * 5).xyz;
    mat4 transform;

    for (int i = 0; i < 4; ++i)
    {
        transform[i] = texelFetch(lightTex, index * 5 + i + 1);
    }

    return Light(radiance, transform, inverse(transform));
}

int getMaterialIndex(int triangleId)
{
    return int(texelFetch(materialMapTex, triangleId)[0]);
}

Material getMaterial(int index)
{
    int offset = index * MATERIAL_SIZE;
    vec3 albedo = vec3(
        texelFetch(materialTex, offset + ALBEDO + 0)[0],
        texelFetch(materialTex, offset + ALBEDO + 1)[0],
        texelFetch(materialTex, offset + ALBEDO + 2)[0]
        );

    float roughness = texelFetch(materialTex, offset + ROUGHNESS)[0];
    float metallic = texelFetch(materialTex, offset + METALLIC)[0];

    return Material(albedo, roughness, metallic);
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

void coordinateSystem(in vec3 v1, out vec3 v2, out vec3 v3)
{
    if (abs(v1.x) > abs(v1.y))
    {
        v2 = vec3(-v1.z, 0.f, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
    }
    else
    {
        v2 = vec3(0.f, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
    }
    v3 = cross(v1, v2);
}

mat3 localToWorld(vec3 normal)
{
    vec3 tangent, bitangent;
    coordinateSystem(normal, tangent, bitangent);
    return mat3(tangent, bitangent, normal);
}

mat3 worldToLocal(vec3 normal)
{
    return transpose(localToWorld(normal));
}

vec3 squareToDiskConcentric(vec2 xi)
{
    vec2 uv = xi * 2.f - vec2(1.f);
    float x2 = uv.x * uv.x;
    float y2 = uv.y * uv.y;

    vec2 polar = vec2(0.f);
    if(x2 > y2)
    {
        polar = vec2(uv.x, (PI / 4.f) * uv.y / uv.x);
    }
    else if (x2 <= y2 && y2 > 0.f)
    {
        polar = vec2(uv.y, (PI / 2.f) - (PI / 4.f) * uv.x / uv.y);
    }

    return vec3(
        cos(polar.y) * polar.x,
        sin(polar.y) * polar.x,
        0.f
        );
}

vec3 squareToHemisphereCosine(vec2 xi)
{
    vec3 hemisphereSample = squareToDiskConcentric(xi);
    hemisphereSample.z = sqrt(max(0.f, 1.f - hemisphereSample.x * hemisphereSample.x - hemisphereSample.y * hemisphereSample.y));
    return hemisphereSample;
}

float hemisphereCosinePDF(vec3 hemisphereSample)
{
    return hemisphereSample.z * INV_PI;
}

// from ShaderToy https://www.shadertoy.com/view/4tXyWN
uvec2 seed;
float rng()
{
    seed += uvec2(1);
    uvec2 q = 1103515245U * ((seed >> 1U) ^ seed.yx);
    uint n = 1103515245U * ((q.x) ^ (q.y >> 3U));
    return float(n) * (1.f / float(0xffffffffU));
}

const float FOVY = 19.5f * PI / 180.f;
Ray raycast()
{
    vec2 offset = vec2(rng(), rng());
    vec2 screenCoords = (gl_FragCoord.xy + offset) / resolution;
    screenCoords = screenCoords * 2.f - vec2(1.f);

    float aspectRatio = float(resolution.x) / resolution.y;
    vec3 ref = eye + forward;
    vec3 V = up * tan(FOVY * 0.5f);
    vec3 H = right * tan(FOVY * 0.5f) * aspectRatio;
    vec3 p = ref + H * screenCoords.x + V * screenCoords.y;

    return Ray(eye, normalize(p - eye));
}

bool rectangleIntersect(Ray ray, mat4 inverseTransform, out float t)
{
    // default rectangle
    // vec3 position = vec3(0.f, 0.f, 0.f);
    // vec3 normal = vec3(0.f, 0.f, 1.f);
    float halfLength = 0.5f;

    ray.origin = vec3(inverseTransform * vec4(ray.origin, 1.f));
    ray.direction = vec3(inverseTransform * vec4(ray.direction, 0.f));
    float dt = -ray.direction.z;

    // One-sided rectangles only
    if (dt < 0.f) return false;
    t = ray.origin.z / dt;
    if (t < 0.f) return false;

    vec3 p = ray.origin + ray.direction * t;

    return (abs(p.x) <= halfLength && abs(p.y) <= halfLength);
}

bool intersect(Ray ray, out Intersection intersection)
{
    intersection.t = 1.f / 0.f;
    intersection.type = -1;
    intersection.index = -1;

    // Geometry
    for (int i = 0; i < indexCount; i++)
    {
        vec4 triangle = texelFetch(indicesTex, i);

        vec3 v0 = texelFetch(verticesTex, int(triangle.x)).xyz;
        vec3 v1 = texelFetch(verticesTex, int(triangle.y)).xyz;
        vec3 v2 = texelFetch(verticesTex, int(triangle.z)).xyz;

        vec3 e0 = v1 - v0;
        vec3 e1 = v2 - v0;

        vec3 n = normalize(cross(e0, e1));

        float sample_t = (dot(n, v0) - dot(n, ray.origin)) / dot(n, ray.direction);
        if (sample_t < 0.f || sample_t > intersection.t) continue;

        vec3 Q = ray.origin + sample_t * ray.direction;

        if (dot(cross(v1 - v0, Q - v0), n) < 0.f) continue;
        if (dot(cross(v2 - v1, Q - v1), n) < 0.f) continue;
        if (dot(cross(v0 - v2, Q - v2), n) < 0.f) continue;

        intersection.t = sample_t;
        intersection.index = i;
        intersection.type = GEOMETRY;
    }

    int lightIndex = 0;
    for (; lightIndex < lightCount[0]; ++lightIndex)
    {
        Light areaLight = getLight(lightIndex);

        float sample_t;
        if (!rectangleIntersect(ray, areaLight.invTransform, sample_t)) continue;
        if (sample_t < 0.f || sample_t > intersection.t) continue;

        intersection.t = 0.f;
        intersection.index = lightIndex;
        intersection.type = LIGHT;
    }

    if (intersection.index == -1) return false;

    if (intersection.type == GEOMETRY)
    {
        vec4 triangle = texelFetch(indicesTex, intersection.index);
        int dataInd = int(triangle.w);
    
        vec3 p = ray.origin + ray.direction * intersection.t;
    
        vec3 v0 = texelFetch(verticesTex, int(triangle.x)).xyz;
        vec3 v1 = texelFetch(verticesTex, int(triangle.y)).xyz;
        vec3 v2 = texelFetch(verticesTex, int(triangle.z)).xyz;
    
        vec3 bary = barycentricCoordinate(p, v0, v1, v2);
    
        vec3 n1 = getVertexAttribute(dataInd, 0, ATTRIBUTE_NORMAL);
        vec3 n2 = getVertexAttribute(dataInd, 1, ATTRIBUTE_NORMAL);
        vec3 n3 = getVertexAttribute(dataInd, 2, ATTRIBUTE_NORMAL);
        intersection.normal = bary.x * n1 + bary.y * n2 + bary.z * n3;

        if (dot(intersection.normal, ray.direction) > 0.f)
        {
            intersection.normal *= -1;
        }
    
        return true;
    }
    else if (intersection.type == LIGHT)
    {
        Light light = getLight(intersection.index);
        intersection.normal = (light.transform * vec4(0.f, 0.f, 1.f, 0.f)).xyz;
        intersection.radiance = light.radiance;
        return true;
    }

    return false;
}

vec3 sampleSurface(Intersection intersection, vec2 xi, vec3 outDir, out vec3 inDir, out float pdf)
{
    Material material = getMaterial(getMaterialIndex(intersection.index));

    vec3 localInDir = squareToHemisphereCosine(xi);
    inDir = localToWorld(intersection.normal) * localInDir;
    pdf = hemisphereCosinePDF(localInDir);

    return material.albedo * INV_PI;
}

void main()
{
    seed = uvec2(iterationCount + 1, iterationCount + 2) * uvec2(gl_FragCoord.xy);
    Ray ray = raycast();

    out_color = vec4(0.f, 0.f, 0.f, 1.f);
    Intersection intersection;

    vec3 attenuation = vec3(1.f);
    vec3 intensity = vec3(0.f);
    for (int i = 0; i < 10; ++i)
    {
        if (!intersect(ray, intersection)) break;

        if (intersection.type == LIGHT)
        {
            intensity = intersection.radiance;
            break;
        }

        vec2 xi = vec2(rng(), rng());
        vec3 inDir;
        float pdf;

        attenuation *= sampleSurface(intersection, xi, -ray.direction, inDir, pdf) * dot(intersection.normal, inDir);

        if (pdf <= 0.f)
        {
            attenuation = vec3(0.f);
            break;
        }
        attenuation /= pdf;

        ray = Ray(ray.origin + ray.direction * intersection.t + inDir * 0.0001f, inDir);
    }

    vec3 accumCol = texture(accumTexture, texCoords).rgb;
    vec3 passCol = attenuation * intensity;
    vec3 col = (accumCol * iterationCount + passCol) / (iterationCount + 1);

    out_color = vec4(col, 1.f);
}
