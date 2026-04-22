#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <string>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 texCoords;
	uint8_t jointIndices[4] = {0};
	float jointWeights[4] = {0.0f};
};

struct BoneData {
	uint32_t parentIndex = -1;
	glm::mat4 localBindPose = glm::mat4(1.0f);
	glm::mat4 worldBindPose = glm::mat4(1.0f);
	glm::mat4 inverseBindPose = glm::mat4(1.0f);
};

struct MeshData {
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<std::string> textures;
	std::vector<BoneData> bones;
};

/**
 * Loads mesh data from a custom binary file format from a game I worked on (SFJ).
 */
bool MeshData_LoadFromSfjFile(MeshData& meshData, const char* filename);

bool MeshData_SaveToGltfFile(const MeshData& meshData, const char* filename, const char* diffuseTextureFilename, const char* normalTextureFilename);
