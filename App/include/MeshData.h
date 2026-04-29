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
	std::vector<uint32_t> childIndices;
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

struct AnimationKeyframe {
	float keyTime = 0.0f;
	float duration = 0.0f;
	uint32_t parentIndex = -1;
	std::vector<uint32_t> childIndices;
	glm::mat4 localTransform = glm::mat4(1.0f);
	glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct AnimationBone {
	std::vector<AnimationKeyframe> keyframes;
};

struct AnimationData {
	std::string name;
	float startTime = 0.0f;
	float endTime = 0.0f;
	std::vector<AnimationBone> bones;
};

/**
 * Loads mesh data from a custom binary file format from a game I worked on (SFJ).
 */
bool MeshData_LoadFromSfjFile(MeshData& meshData, const char* filename);

bool AnimationData_LoadFromSfjFile(AnimationData& animData, const char* filename);

bool MeshData_SaveToGltfFile(const MeshData& meshData, const char* filename,
	const char* diffuseTextureFilename, const char* normalTextureFilename, const char* animationFilename);
