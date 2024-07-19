#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

#include <tiny_obj_loader.h>

#include <vector>
#include <iostream>

class Scene
{
public:
	struct VertexData {
		VertexData(glm::vec3 normal, glm::vec2 textureCoordinates)
			: normal(normal), textureCoordinates(glm::vec3(textureCoordinates, 0.f))
		{}

		glm::vec3 normal;
		glm::vec3 textureCoordinates;
	};

	struct Light {
		Light(glm::vec3 radiance, glm::mat4 transform)
			: radiance(vec4(radiance, 0.f)), transform(transform)
		{}

		Light(glm::vec3 radiance, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
			: Light(radiance, glm::translate(glm::mat4(1.f), position) * glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z) * glm::scale(glm::mat4(1.f), scale))
		{}

		vec4 radiance;
		glm::mat4 transform;
	};

	struct Material {
		Material(glm::vec3 albedo, float roughness, float metallic, float ior, float anisotropy, float transmission)
			: albedo(albedo), roughness(roughness), metallic(metallic), ior(ior), anisotropy(anisotropy), transmission(transmission)
		{}

		glm::vec3 albedo;
		float roughness;
		float metallic;
		float ior;
		float anisotropy;
		float transmission;
	};

	std::vector<glm::vec3> m_vertices;
	std::vector<VertexData> m_vertexData;

	std::vector<glm::vec4> m_indices;

	std::vector<Light> m_lights;
	glm::uvec4 m_lightCount;

	std::vector<Material> m_materials;
	std::vector<uint32_t> m_materialMap;

	Scene()
		: m_vertices(std::vector<glm::vec3> {glm::vec3(-1.f, -1.f, 0.f), glm::vec3(1.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f)}),
		  m_vertexData(std::vector<VertexData> {
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f)),
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f)),
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f))
		  }),
		  m_indices(std::vector<glm::vec4> {glm::vec4(0, 1, 2, 0)}),
		  m_lights(std::vector<Light> {
		  	Light(glm::vec3(1.f), glm::vec3(0.f, 2.f, 0.f), glm::vec3(3.14f / 2.f, 0.f, 0.f), glm::vec3(2.f, 1.f, 1.f))
		  }),
		  m_lightCount(1, 0, 0, 0)
	{ }

	// Load the scene as an obj file
	Scene(const char* objFilename, const char* mtlRoot = nullptr)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if(!LOG_IF_ERROR(tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objFilename, mtlRoot, true)))
		{
			std::cout << err << std::endl;
			return;
		}

		// Vertices
		size_t vertexCount = attrib.vertices.size() / 3;
		m_vertices.reserve(vertexCount);
		for (size_t i = 0; i < vertexCount; ++i)
		{
			m_vertices.push_back(glm::vec3(attrib.vertices[3 * i], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]));
		}

		// Indices
		size_t indexOffset = 0;
		for (size_t s = 0; s < shapes.size(); ++s)
		{
			indexOffset = m_indices.size();
			m_indices.reserve(indexOffset + shapes[s].mesh.indices.size());

			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
			{
				// There are always 3 vertices per polygon with triangulation enabled
				glm::vec4 indices;
				for (int i = 0; i < 3; ++i)
				{
					tinyobj::index_t objIndex = tinyobj::index_t(shapes[s].mesh.indices[f * 3 + i]);

					// Index
					indices[i] = objIndex.vertex_index;

					// Normal
					glm::vec3 normal = glm::vec3(0.f, 0.f, -1.f);
					if (objIndex.normal_index != -1)
					{
						normal = glm::vec3(
							attrib.normals[3 * objIndex.normal_index + 0],
							attrib.normals[3 * objIndex.normal_index + 1],
							attrib.normals[3 * objIndex.normal_index + 2]
							);
					}

					m_vertexData.push_back(VertexData(normal, glm::vec2(0.f)));
				}
				indices.w = indexOffset + f;

				m_materialMap.push_back(shapes[s].mesh.material_ids[f]);
				m_indices.push_back(indices);
			}
		}

		// Materials
		for (size_t i = 0; i < materials.size(); ++i)
		{
			const tinyobj::material_t& mat = materials[i];

			// Illumination model
			bool doHighlight = false; UNUSED(doHighlight);
			bool doReflection = false;
			switch (mat.illum)
			{
			case 1:
				// Base color and Ambient
				break;
			case 2:
				// Specular Highlights
				doHighlight = true;
				break;
			case 3:
				// Reflections
				doReflection = true;
				break;
			}

			glm::vec3 albedo;
			float roughness, metallic, ior, anisotropy, transmission;

			// Albedo
			albedo = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);

			// IOR
			ior = mat.ior;

			if (!mat.isPBR)
			{
				// Roughness
				roughness = 1.f;
				// Same shininess to roughness transform used by Blender
				if (mat.shininess < 0.f && doHighlight)
				{
					roughness = 0.f;
				}
				else
				{
					float clampedShininess = std::max(0.f, std::min(mat.shininess, 1000.f));
					roughness = 1.f - std::sqrt(clampedShininess / 1000.f);
				}

				// Metallic
				metallic = 0.f;
				if (doReflection)
				{
					metallic = (mat.ambient[0] + mat.ambient[1] + mat.ambient[2]) / 3.f;
					if (metallic < 0.f) metallic = 1.f;
				}

				// Anisotropy
				anisotropy = 0.f;

				// Transmission
				transmission = 0.f;
			}
			else
			{
				// PBR extension overrides

				// Roughness
				roughness = mat.roughness;

				// Metallic
				metallic = mat.metallic;

				// Anisotropy
				anisotropy = mat.anisotropy;

				// Transmission
				transmission = (mat.transmittance[0] + mat.transmittance[1] + mat.transmittance[2]) / 3.f;
			}

			m_materials.push_back(Material(albedo, roughness, metallic, ior, anisotropy, transmission));
		}
	}

	Scene(const std::string filename)
		: Scene(filename.c_str())
	{ }
};
