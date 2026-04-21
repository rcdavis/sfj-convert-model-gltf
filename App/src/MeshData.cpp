#include "MeshData.h"

#include <fstream>
#include "Utils/Log.h"

#include "glm/gtc/type_ptr.hpp"
#include "tiny_gltf.h"

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

bool MeshData_SaveToGltfFile(const MeshData& meshData, const char* filename) {
	LOG_INFO("Saving mesh to GLTF file: {}", filename);

	tinygltf::Model model;

	// Vertex buffer
	tinygltf::Buffer vertexBuffer;
	vertexBuffer.data.resize(meshData.vertices.size() * sizeof(Vertex));
	memcpy(vertexBuffer.data.data(), meshData.vertices.data(), vertexBuffer.data.size());
	model.buffers.emplace_back(std::move(vertexBuffer));

	// Index buffer
	tinygltf::Buffer indexBuffer;
	indexBuffer.data.resize(meshData.indices.size() * sizeof(uint16_t));
	memcpy(indexBuffer.data.data(), meshData.indices.data(), indexBuffer.data.size());
	model.buffers.emplace_back(std::move(indexBuffer));

	// Vertex buffer view
	tinygltf::BufferView vertexView;
	vertexView.buffer = 0; // vertex buffer index
	vertexView.byteOffset = 0;
	vertexView.byteLength = meshData.vertices.size() * sizeof(Vertex);
	vertexView.byteStride = sizeof(Vertex);
	model.bufferViews.emplace_back(std::move(vertexView));

	// Index buffer view
	tinygltf::BufferView indexView;
	indexView.buffer = 1; // index buffer index
	indexView.byteOffset = 0;
	indexView.byteLength = meshData.indices.size() * sizeof(uint16_t);
	indexView.byteStride = sizeof(uint16_t);
	model.bufferViews.emplace_back(std::move(indexView));

	// Calculate min and max values for position attribute
	glm::vec3 minPos(std::numeric_limits<float>::max());
	glm::vec3 maxPos(-std::numeric_limits<float>::max());
	for (const auto& vertex : meshData.vertices) {
		minPos = glm::min(minPos, vertex.pos);
		maxPos = glm::max(maxPos, vertex.pos);
	}

	// Vertex Position accessor
	tinygltf::Accessor posAccessor;
	posAccessor.bufferView = 0; // vertex buffer view index
	posAccessor.byteOffset = offsetof(Vertex, pos);
	posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	posAccessor.count = meshData.vertices.size();
	posAccessor.type = TINYGLTF_TYPE_VEC3;
	posAccessor.minValues = { minPos.x, minPos.y, minPos.z };
	posAccessor.maxValues = { maxPos.x, maxPos.y, maxPos.z };
	model.accessors.emplace_back(std::move(posAccessor));

	// Vertex Normal accessor
	tinygltf::Accessor normalAccessor;
	normalAccessor.bufferView = 0; // vertex buffer view index
	normalAccessor.byteOffset = offsetof(Vertex, normal);
	normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	normalAccessor.count = meshData.vertices.size();
	normalAccessor.type = TINYGLTF_TYPE_VEC3;
	model.accessors.emplace_back(std::move(normalAccessor));

	// Vertex Tangent accessor
	tinygltf::Accessor tangentAccessor;
	tangentAccessor.bufferView = 0; // vertex buffer view index
	tangentAccessor.byteOffset = offsetof(Vertex, tangent);
	tangentAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	tangentAccessor.count = meshData.vertices.size();
	tangentAccessor.type = TINYGLTF_TYPE_VEC3;
	model.accessors.emplace_back(std::move(tangentAccessor));

	// Vertex Texture Coordinates accessor
	tinygltf::Accessor texCoordsAccessor;
	texCoordsAccessor.bufferView = 0; // vertex buffer view index
	texCoordsAccessor.byteOffset = offsetof(Vertex, texCoords);
	texCoordsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	texCoordsAccessor.count = meshData.vertices.size();
	texCoordsAccessor.type = TINYGLTF_TYPE_VEC2;
	model.accessors.emplace_back(std::move(texCoordsAccessor));

	// Index accessor
	tinygltf::Accessor indexAccessor;
	indexAccessor.bufferView = 1; // index buffer view index
	indexAccessor.byteOffset = 0;
	indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	indexAccessor.count = meshData.indices.size();
	indexAccessor.type = TINYGLTF_TYPE_SCALAR;
	model.accessors.emplace_back(std::move(indexAccessor));

	// Primitive
	tinygltf::Primitive primitive;
	primitive.attributes["POSITION"] = 0; // position accessor index
	primitive.attributes["NORMAL"] = 1;   // normal accessor index
	primitive.attributes["TANGENT"] = 2;  // tangent accessor index
	primitive.attributes["TEXCOORD_0"] = 3; // texCoords accessor index
	primitive.indices = 4; // index accessor index
	primitive.mode = TINYGLTF_MODE_TRIANGLES;

	// Mesh
	tinygltf::Mesh mesh;
	mesh.primitives.emplace_back(std::move(primitive));
	model.meshes.emplace_back(std::move(mesh));

	// Node
	tinygltf::Node node;
	node.mesh = 0; // mesh index
	model.nodes.emplace_back(std::move(node));

	// Scene
	tinygltf::Scene scene;
	scene.nodes.emplace_back(0); // node index
	model.scenes.emplace_back(std::move(scene));
	model.defaultScene = 0;

	// Save the model to a GLTF file
	tinygltf::TinyGLTF gltfContext;
	if (!gltfContext.WriteGltfSceneToFile(&model, filename, true, true, true, false)) {
		LOG_ERROR("Failed to save GLTF file: {}", filename);
		return false;
	}

	return true;
}
