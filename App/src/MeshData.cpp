#include "MeshData.h"

#include <fstream>
#include "Utils/Log.h"

#include "glm/gtc/type_ptr.hpp"

bool MeshData_LoadFromSfjFile(MeshData& meshData, const char* filename) {
	LOG_INFO("Loading mesh from file: {}", filename);

	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		LOG_ERROR("Failed to open file: {}", filename);
		return false;
	}

	uint32_t textureCount = 0;
	file.read((char*)&textureCount, sizeof(textureCount));

	for (uint32_t i = 0; i < textureCount; ++i) {
		uint32_t nameLength = 0;
		file.read((char*)&nameLength, sizeof(nameLength));

		assert(nameLength < 256);

		char textureName[256] = {0};
		file.read(textureName, nameLength);

		meshData.textures.emplace_back(textureName);
	}

	uint32_t vertexCount = 0;
	file.read((char*)&vertexCount, sizeof(vertexCount));

	meshData.vertices.reserve(vertexCount);
	for (uint32_t i = 0; i < vertexCount; ++i) {
		Vertex vertex;

		file.read((char*)glm::value_ptr(vertex.pos), sizeof(glm::vec3));
		file.read((char*)glm::value_ptr(vertex.normal), sizeof(glm::vec3));
		file.read((char*)glm::value_ptr(vertex.tangent), sizeof(glm::vec3));
		file.read((char*)glm::value_ptr(vertex.texCoords), sizeof(glm::vec2));

		uint32_t influenceCount = 0;
		file.read((char*)&influenceCount, sizeof(uint32_t));

		uint32_t joints[8] = {0};
		float weights[8] = {0.0f};

		for (uint32_t curInfluence = 0; curInfluence < influenceCount; ++curInfluence) {
			file.read((char*)(joints + curInfluence), sizeof(uint32_t));
			file.read((char*)(weights + curInfluence), sizeof(float));
		}

		// TODO: Store joint influences in the vertex data

		meshData.vertices.emplace_back(vertex);
	}

	uint32_t primitivesCount = 0;
	file.read((char*)&primitivesCount, sizeof(uint32_t));

	meshData.indices.resize(primitivesCount * 3);
	std::vector<uint32_t> tempIndices(primitivesCount * 3);
	file.read((char*)tempIndices.data(), tempIndices.size() * sizeof(uint32_t));

	for (size_t i = 0; i < tempIndices.size(); ++i) {
		meshData.indices[i] = static_cast<uint16_t>(tempIndices[i]);
	}

	return true;
}
