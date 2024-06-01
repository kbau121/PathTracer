#pragma once

#include <glm/glm.hpp>

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

	std::vector<glm::vec3> m_vertices;
	std::vector<VertexData> m_vertexData;

	std::vector<glm::vec4> m_indices;

	Scene()
		: m_vertices(std::vector<glm::vec3> {glm::vec3(-1.f, -1.f, 5.f), glm::vec3(1.f, -1.f, 5.f), glm::vec3(0.f, 1.f, 5.f)}),
		  m_vertexData(std::vector<VertexData> {
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f)),
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f)),
		  	VertexData(glm::vec3(0.f, 0.f, -1.f), glm::vec2(0.f))
		  }),
		  m_indices(std::vector<glm::vec4> {glm::vec4(0, 1, 2, 0)})
	{ }

	// Load the scene as an obj file
	Scene(const char* filename)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if(!LOG_IF_ERROR(tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, nullptr, true)))
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

				m_indices.push_back(indices);
			}
		}
	}

	Scene(const std::string filename)
		: Scene(filename.c_str())
	{ }
};
