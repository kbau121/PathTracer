#version 460

#define PI      3.14159265358979323
#define INV_PI  0.31830988618379067
#define IOR_AIR 1.0003

// Vertex attributes
#define NUM_VERTEX_ATTRIBUTES 2
#define ATTRIBUTE_NORMAL 0
#define ATTRIBUTE_TEXTURE_COORDINATE 1

// Material attributes
#define ALBEDO                      0
#define ROUGHNESS     (ALBEDO     + 3)
#define METALLIC      (ROUGHNESS  + 1)
#define IOR           (METALLIC   + 1)
#define ANISOTROPY    (IOR        + 1)
#define MATERIAL_SIZE (ANISOTROPY + 1)

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
    float ior;
    float anisotropy;
};

// ==================
// == Data Getters ==
// ==================

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
    float ior = texelFetch(materialTex, offset + IOR)[0];
    float anisotropy = texelFetch(materialTex, offset + ANISOTROPY)[0];

    return Material(albedo, roughness, metallic, ior, anisotropy);
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

// =======================================
// == Coordinate System transformations ==
// =======================================

// Compute tangent and bitangent
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

// Local space assumes a normal of (0, 0, 1)

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

// ========================================
// == Local space trigonometry functions ==
// ========================================

float cosTheta(vec3 v)
{
    return v.z;
}

float cos2Theta(vec3 v)
{
    return v.z * v.z;
}

float sin2Theta(vec3 v)
{
    return max(0.f, 1.f - cos2Theta(v));
}

float sinTheta(vec3 v)
{
    return sqrt(sin2Theta(v));
}

float tanTheta(vec3 v)
{
    return sinTheta(v) / cosTheta(v);
}

float tan2Theta(vec3 v)
{
    return sin2Theta(v) / cos2Theta(v);
}

float cosPhi(vec3 v)
{
    float sinTheta = sinTheta(v);
    return (sinTheta == 0) ? 1 : clamp(v.x / sinTheta, -1.f, 1.f);
}

float sinPhi(vec3 v)
{
    float sinTheta = sinTheta(v);
    return (sinTheta == 0) ? 0 : clamp(v.y / sinTheta, -1.f, 1.f);
}

float cos2Phi(vec3 v)
{
    float cosPhi = cosPhi(v);
    return cosPhi * cosPhi;
}

float sin2Phi(vec3 v)
{
    float sinPhi = sinPhi(v);
    return sinPhi * sinPhi;
}

// ========================
// == Sampling Functions ==
// ========================

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

vec2 squareToUniformDiskPolar(vec2 xi)
{
    float r = sqrt(xi.x);
    float theta = 2 * PI * xi.y;
    return vec2(r * cos(theta), r * sin(theta));
}

vec3 squareToHemisphereCosine(vec2 xi)
{
    vec3 hemisphereSample = squareToDiskConcentric(xi);
    hemisphereSample.z = sqrt(max(0.f, 1.f - hemisphereSample.x * hemisphereSample.x - hemisphereSample.y * hemisphereSample.y));
    return hemisphereSample;
}

// =======================
// == Utility Functions ==
// =======================

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

// Complex number operations

// From https://gist.github.com/DonKarlssonSan/f87ba5e4e5f1093cb83e39024a6a5e72
vec2 cxSqrt(vec2 a)
{
    float r = length(a);
    float realPart = sqrt(0.5f * (r + a.x));
    float imagPart = sqrt(max(0.f, 0.5f * (r - a.x)));
    if (a.y < 0.f) imagPart = -imagPart;
    return vec2(realPart, imagPart);
}

vec2 cxMul(vec2 a, vec2 b)
{
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec2 cxDiv(vec2 a, vec2 b)
{
    return vec2(((a.x * b.x + a.y * b.y) / (b.x * b.x + b.y * b.y)), ((a.y * b.x - a.x * b.y) / (b.x * b.x + b.y * b.y)));
}

// ============================
// == Intersection Functions ==
// ============================

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

// =================================
// == Microfacet Helper Functions ==
// =================================

vec2 roughnessToAnisotropicWeights(float roughness, float anisotropic)
{
    float aspect = sqrt(1.f - 0.9f * anisotropic);
    vec2 anisotropicWeights = vec2(roughness * roughness);

    anisotropicWeights.x /= aspect;
    anisotropicWeights.y *= aspect;

    return anisotropicWeights;
}

float isotropicRoughness2(vec3 v, vec2 weights)
{
    return
        weights.x * weights.x * cos2Phi(v)
        + weights.y * weights.y * sin2Phi(v);
}

float isotropicRoughness(vec3 v, vec2 weights)
{
    return sqrt(isotropicRoughness2(v, weights));
}

float trowbridgeReitzDistribution(vec3 halfVector, vec2 weights)
{
    float tan2Theta = tan2Theta(halfVector);
    if (isinf(tan2Theta)) return 0.f;

    float cos2Theta = cos2Theta(halfVector);
    float cos4Theta = cos2Theta * cos2Theta;
    float e =
        (
          cos2Phi(halfVector) / (weights.x * weights.x)
          + sin2Phi(halfVector) / (weights.y * weights.y)
        ) * tan2Theta;

    return 1.f / (PI * weights.x * weights.y * cos4Theta * (1.f + e) * (1.f + e));
}

float trowbridgeReitzLambda(vec3 v, vec2 weights)
{
    float tan2Theta = tan2Theta(v);
    if (isinf(tan2Theta)) return 0.f;

    return (sqrt(1.f + isotropicRoughness2(v, weights) * tan2Theta) - 1.f) * 0.5f;
}

float trowbridgeReitzMasking(vec3 outDir, vec3 inDir, vec2 weights)
{
    return 1.f / (1.f + trowbridgeReitzLambda(outDir, weights) + trowbridgeReitzLambda(inDir, weights));
}

vec3 trowbridgeReitzSampleNormal(vec3 localOutDir, vec2 xi, vec2 weights)
{
    vec3 hemisphereOutDir = normalize(vec3(weights.x * localOutDir.x, weights.y * localOutDir.y, localOutDir.z));
    if (hemisphereOutDir.z < 0) hemisphereOutDir = -hemisphereOutDir;

    vec3 T1 = (hemisphereOutDir.z < 0.99999f) ? normalize(cross(vec3(0.f, 0.f, 1.f), hemisphereOutDir)) : vec3(1.f, 0.f, 0.f);
    vec3 T2 = cross(hemisphereOutDir, T1);

    vec2 p = squareToUniformDiskPolar(xi);

    float h = sqrt(1.f - p.x * p.x);
    p.y = mix((1.f - hemisphereOutDir.z) / 2.f, h, p.y);

    float pz = sqrt(max(0.f, 1.f - dot(p, p)));
    vec3 hemisphereNormal = p.x * T1 + p.y * T2 + pz * hemisphereOutDir;
    return normalize(vec3(weights.x * hemisphereNormal.x, weights.y * hemisphereNormal.y, max(0.000001f, hemisphereNormal.z)));
}

float trowbridgeReitzPdf(vec2 roughness, vec3 localOutDir, vec3 halfVector)
{
    return trowbridgeReitzDistribution(halfVector, roughness) * abs(cosTheta(halfVector)) / (4.f * dot(localOutDir, halfVector));
}

// ===========================
// == BxDF Helper Functions ==
// ===========================

float colorToLuminance(vec3 color)
{
    return 0.2126f * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

vec3 colorToTint(vec3 color)
{
    float luminance = colorToLuminance(color);
    return color / luminance;
}

vec3 schlickR0(float eta)
{
    float R0 = (eta - 1.f) / (eta + 1.f);
    R0 = R0 * R0;
    return vec3(R0);
}

vec3 schlickR0(float ior1, float ior2)
{
    float R0 = (ior1 - ior2) / (ior1 + ior2);
    R0 = R0 * R0;
    return vec3(R0);
}

vec3 schlickFresnel(vec3 R0, float cosThetaDiff)
{
    return R0 + (vec3(1.f) - R0) * (1 - cosThetaDiff);
}

vec3 fresnelDielectric(float cosThetaIn, float eta)
{
    // Flip the orientation if backwards
    if (cosThetaIn < 0.f)
    {
        eta = 1.f / eta;
        cosThetaIn = -cosThetaIn;
    }

    // Find the cosine of the transmitted direction according to Snell's law
    float sin2ThetaIn = 1 - cosThetaIn * cosThetaIn;
    float sin2ThetaTran = sin2ThetaIn / (eta * eta);

    // Total interanl refraction
    if (sin2ThetaTran >= 1.f) return vec3(1.f);

    float cosThetaTran = sqrt(1.f - sin2ThetaTran);

    float refPara = (eta * cosThetaIn - cosThetaTran) / (eta * cosThetaIn + cosThetaTran);
    float refPerp = (cosThetaIn - eta * cosThetaTran) / (cosThetaIn + eta * cosThetaTran);

    float fresnel = (refPara * refPara + refPerp * refPerp) / 2.f;
    return vec3(fresnel);
}

vec3 fresnelDielectric(float cosThetaIn, float ior1, float ior2)
{
    return fresnelDielectric(cosThetaIn, ior1 / ior2);
}

vec3 disneyFresnel(Material material, vec3 localOutDir, vec3 halfVector, vec3 localInDir)
{
    float cosThetaOut = abs(dot(halfVector, localOutDir));
    float cosThetaIn = dot(halfVector, localInDir);

    // TODO add specular tint to material properties
    vec3 tint = colorToTint(material.albedo);

    vec3 R0 = schlickR0(IOR_AIR, material.ior);
    R0 = mix(R0, material.albedo, material.metallic);

    vec3 dielectricFresnel = fresnelDielectric(cosThetaIn, IOR_AIR, material.ior);
    vec3 metallicFresnel = schlickFresnel(R0, cosThetaIn);
    return mix(dielectricFresnel, metallicFresnel, material.metallic);
}

// ====================
// == BxDF Functions ==
// ====================

// Diffuse

vec3 diffuseAttenuation(Intersection intersection, Material material, vec3 localOutDir, vec3 halfVector, vec3 localInDir)
{
    // Lambertion
    vec3 lambert = material.albedo * INV_PI;

    // Retro-Reflection
    float cosThetaOut = cosTheta(localOutDir);
    float cosThetaIn = cosTheta(localInDir);
    float cosThetaDiff = dot(localOutDir, halfVector);

    float retroWeight = 0.5f + 2.f * material.roughness * cosThetaDiff * cosThetaDiff;

    float retroResponse = (1.f + retroWeight * pow(1.f - cosThetaOut, 5.f)) * (1.f + retroWeight * pow(1.f - cosThetaIn, 5.f));

    return lambert * retroResponse;
}

vec3 diffuseBRDF(Intersection intersection, Material material, vec3 localOutDir, vec3 halfVector, vec3 localInDir, out float pdf)
{
    // PDF
    pdf = abs(cosTheta(localInDir));

    // Attenuation
    return diffuseAttenuation(intersection, material, localOutDir, halfVector, localInDir);
}

vec3 sampleDiffuse(Intersection intersection, Material material, vec2 xi, vec3 outDir, out vec3 inDir, out float pdf)
{
    mat3 lToW = localToWorld(intersection.normal);
    mat3 wToL = transpose(lToW);

    // Find the incoming light direction
    vec3 localOutDir = wToL * outDir;
    vec3 localInDir = squareToHemisphereCosine(xi);
    vec3 halfVector = normalize(localOutDir + localInDir);

    inDir = lToW * localInDir;

    // Compute the BRDF
    return diffuseBRDF(intersection, material, localOutDir, halfVector, localInDir, pdf);
}

// Specular

vec3 specularAttenuation(Intersection intersection, Material material, vec3 localOutDir, vec3 halfVector, vec3 localInDir)
{
    float cosThetaOut = abs(cosTheta(localOutDir));
    float cosThetaIn = abs(cosTheta(localInDir));

    if (cosThetaIn <= 0.f || cosThetaOut <= 0.f) return vec3(0.f);

    vec2 anisotropicWeights = roughnessToAnisotropicWeights(material.roughness, material.anisotropy);

    vec3 fresnel = disneyFresnel(material, localOutDir, halfVector, localInDir);

    float distribution = trowbridgeReitzDistribution(halfVector, anisotropicWeights);
    float masking = trowbridgeReitzMasking(localOutDir, localInDir, anisotropicWeights);

    return distribution * masking * fresnel / (4 * cosThetaIn * cosThetaOut);
}

vec3 specularBRDF(Intersection intersection, Material material, vec3 localOutDir, vec3 halfVector, vec3 localInDir, out float pdf)
{
    // PDF
    vec2 anisotropicWeights = roughnessToAnisotropicWeights(material.roughness, material.anisotropy);
    pdf = trowbridgeReitzPdf(anisotropicWeights, localOutDir, halfVector);

    // Attenuation
    return specularAttenuation(intersection, material, localOutDir, halfVector, localInDir);
}

vec3 sampleSpecular(Intersection intersection, Material material, vec2 xi, vec3 outDir, out vec3 inDir, out float pdf)
{
    mat3 lToW = localToWorld(intersection.normal);
    mat3 wToL = transpose(lToW);

    // Find the incoming light direction
    vec3 localOutDir = wToL * outDir;

    if (localOutDir.z == 0.f) return vec3(0.f);

    vec2 anisotropicWeights = roughnessToAnisotropicWeights(material.roughness, material.anisotropy);

    vec3 halfVector = trowbridgeReitzSampleNormal(localOutDir, xi, anisotropicWeights);
    vec3 localInDir = reflect(-localOutDir, halfVector);

    if (localInDir.z * localOutDir.z <= 0.f) return vec3(0.f);

    inDir = lToW * localInDir;

    // Compute the BRDF
    return specularBRDF(intersection, material, localOutDir, halfVector, localInDir, pdf);
}

// Generic BxDF sampling functions

void getLobeProbabilities(Material material, out float pDiffuse, out float pSpecular, out float pTransmission)
{
    float matSpecularTransmission = 0.f;

    float metallicBRDF = material.metallic;
    float specularBSDF = (1.f - material.metallic) * (matSpecularTransmission);
    float dielectricBRDF = (1.f - material.metallic) * (1.f - matSpecularTransmission);

    float diffuseWeight = dielectricBRDF;
    float specularWeight = metallicBRDF + dielectricBRDF;
    float transmissionWeight = specularBSDF;

    float totalWeightInv = 1.f / (specularWeight + transmissionWeight + diffuseWeight);

    pDiffuse = diffuseWeight * totalWeightInv;
    pSpecular = specularWeight * totalWeightInv;
    pTransmission = transmissionWeight * totalWeightInv;
}

vec3 sampleSurface(Intersection intersection, vec2 xi, vec3 outDir, out vec3 inDir, out float pdf)
{
    Material material = getMaterial(getMaterialIndex(intersection.index));

    float pDiffuse, pSpecular, pTransmission;
    getLobeProbabilities(material, pDiffuse, pSpecular, pTransmission);

    float lobeChoice = rng();

    if (lobeChoice <= pDiffuse)
    {
        vec3 attenuation = sampleDiffuse(intersection, material, xi, outDir, inDir, pdf);
        pdf *= pDiffuse;
        return attenuation;
    }
    else if (lobeChoice <= pSpecular + pDiffuse)
    {
        vec3 attenuation = sampleSpecular(intersection, material, xi, outDir, inDir, pdf);
        pdf *= pSpecular;
        return attenuation;
    }

    return vec3(0.f);
}

// ======================
// == Main Render Loop ==
// ======================

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
