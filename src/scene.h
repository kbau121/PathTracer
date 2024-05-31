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
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_normals;
	std::vector<glm::vec2> m_textureCoordinates;

	std::vector<glm::vec3> m_indices;

	Scene()
		: m_vertices(std::vector<glm::vec3>
			{glm::vec3(-1.f, -1.f, 5.f), glm::vec3(1.f, -1.f, 5.f), glm::vec3(0.f, 1.f, 5.f)}
			),
		  m_indices(std::vector<glm::vec3> {glm::vec3(0, 1, 2)})
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
		for (size_t s = 0; s < shapes.size(); ++s)
		{
			m_indices.reserve(m_indices.size() + shapes[s].mesh.indices.size());

			for (size_t f = 0; f < shapes[s].mesh.indices.size(); ++f)
			{
				// There are always 3 vertices per polygon with triangulation enabled
				glm::vec3 indices;
				for (int i = 0; i < 3; ++i)
				{
					tinyobj::index_t objIndex = tinyobj::index_t(shapes[s].mesh.indices[f * 3 + i]);
					indices[i] = objIndex.vertex_index;
				}
				m_indices.push_back(indices);
			}
		}
	}

	Scene(const std::string filename)
		: Scene(filename.c_str())
	{ }
};
