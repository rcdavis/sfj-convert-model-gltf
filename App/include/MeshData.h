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
	std::vector<uint16_t> indices;
	std::vector<std::string> textures;
};

/**
 * Loads mesh data from a custom binary file format from a game I worked on (SFJ).
 */
bool MeshData_LoadFromSfjFile(MeshData& meshData, const char* filename);

bool MeshData_SaveToGltfFile(const MeshData& meshData, const char* filename);
