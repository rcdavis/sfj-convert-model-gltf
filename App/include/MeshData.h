#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <string>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texCoords;
};

struct MeshData {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<std::string> textures;
};

bool MeshData_LoadFromFile(MeshData& meshData, const char* filename);
