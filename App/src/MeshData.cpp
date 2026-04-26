#include "MeshData.h"

#include <fstream>
#include "Utils/Log.h"

#include "glm/gtc/type_ptr.hpp"
#include "tiny_gltf.h"
#include "stb_image.h"

struct ImageData {
	int width = 0;
	int height = 0;
	int channels = 0;
	std::vector<unsigned char> data;
};

struct VertexInfluence {
	uint32_t jointIndex = 0;
	float weight = 0.0f;
};

static bool ImageData_Load(ImageData& imageData, const char* filename);
static void VertexInfluence_SortAndNormalize(std::vector<VertexInfluence>& influences);
static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& outTranslation,
	glm::quat& outRotation, glm::vec3& outScale);

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

		glm::vec3 tangent;
		file.read((char*)glm::value_ptr(tangent), sizeof(glm::vec3));
		vertex.tangent = glm::vec4(tangent, 1.0f);

		file.read((char*)glm::value_ptr(vertex.texCoords), sizeof(glm::vec2));

		uint32_t influenceCount = 0;
		file.read((char*)&influenceCount, sizeof(uint32_t));

		std::vector<VertexInfluence> influences(influenceCount);
		file.read((char*)influences.data(), influenceCount * sizeof(VertexInfluence));

		VertexInfluence_SortAndNormalize(influences);

		for (size_t curInfluence = 0; curInfluence < influences.size(); ++curInfluence) {
			vertex.jointIndices[curInfluence] = (uint8_t)influences[curInfluence].jointIndex;
			vertex.jointWeights[curInfluence] = influences[curInfluence].weight;
		}

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

	uint32_t bindPoseFrameCount = 0;
	file.read((char*)&bindPoseFrameCount, sizeof(uint32_t));

	meshData.bones.resize(bindPoseFrameCount);

	for (uint32_t i = 0; i < bindPoseFrameCount; ++i) {
		glm::mat4 localBindPose;
		file.read((char*)glm::value_ptr(localBindPose), sizeof(glm::mat4));
		meshData.bones[i].localBindPose = localBindPose;
	}

	for (uint32_t i = 0; i < bindPoseFrameCount; ++i) {
		uint32_t frameIndex = 0;
		file.read((char*)&frameIndex, sizeof(uint32_t));

		uint32_t childrenCount = 0;
		file.read((char*)&childrenCount, sizeof(uint32_t));

		for (uint32_t j = 0; j < childrenCount; ++j) {
			uint32_t childIndex = 0;
			file.read((char*)&childIndex, sizeof(uint32_t));

			meshData.bones[childIndex].parentIndex = frameIndex;
			meshData.bones[frameIndex].childIndices.emplace_back(childIndex);
		}
	}

	// NOTE: This only works if parent is already computed before child, which is the case here
	for (uint32_t i = 0; i < bindPoseFrameCount; ++i) {
		if (meshData.bones[i].parentIndex != -1) {
			meshData.bones[i].worldBindPose = meshData.bones[meshData.bones[i].parentIndex].worldBindPose *
				meshData.bones[i].localBindPose;
		} else {
			meshData.bones[i].worldBindPose = meshData.bones[i].localBindPose;
		}

		meshData.bones[i].inverseBindPose = glm::inverse(meshData.bones[i].worldBindPose);
	}

	return true;
}

bool MeshData_SaveToGltfFile(const MeshData& meshData, const char* filename, const char* diffuseTextureFilename, const char* normalTextureFilename) {
	tinygltf::Model model;

	// Vertex buffer
	tinygltf::Buffer vertexBuffer;
	vertexBuffer.data.resize(meshData.vertices.size() * sizeof(Vertex));
	memcpy(vertexBuffer.data.data(), meshData.vertices.data(), vertexBuffer.data.size());
	const int vertexBufferIndex = static_cast<int>(model.buffers.size());
	model.buffers.emplace_back(std::move(vertexBuffer));

	// Index buffer
	tinygltf::Buffer indexBuffer;
	indexBuffer.data.resize(meshData.indices.size() * sizeof(uint16_t));
	memcpy(indexBuffer.data.data(), meshData.indices.data(), indexBuffer.data.size());
	const int indexBufferIndex = static_cast<int>(model.buffers.size());
	model.buffers.emplace_back(std::move(indexBuffer));

	// Inverse bind pose buffer
	tinygltf::Buffer inverseBindPoseBuffer;
	inverseBindPoseBuffer.data.resize(meshData.bones.size() * sizeof(glm::mat4));
	for (size_t i = 0; i < meshData.bones.size(); ++i) {
		memcpy(inverseBindPoseBuffer.data.data() + i * sizeof(glm::mat4),
			glm::value_ptr(meshData.bones[i].inverseBindPose), sizeof(glm::mat4));
	}
	const int inverseBindPoseBufferIndex = static_cast<int>(model.buffers.size());
	model.buffers.emplace_back(std::move(inverseBindPoseBuffer));

	// Vertex buffer view
	tinygltf::BufferView vertexView;
	vertexView.buffer = vertexBufferIndex;
	vertexView.byteOffset = 0;
	vertexView.byteLength = meshData.vertices.size() * sizeof(Vertex);
	vertexView.byteStride = sizeof(Vertex);
	vertexView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
	const int vertexBufferViewIndex = static_cast<int>(model.bufferViews.size());
	model.bufferViews.emplace_back(std::move(vertexView));

	// Index buffer view
	tinygltf::BufferView indexView;
	indexView.buffer = indexBufferIndex;
	indexView.byteOffset = 0;
	indexView.byteLength = meshData.indices.size() * sizeof(uint16_t);
	indexView.byteStride = sizeof(uint16_t);
	indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	const int indexBufferViewIndex = static_cast<int>(model.bufferViews.size());
	model.bufferViews.emplace_back(std::move(indexView));

	// Inverse bind pose buffer view
	tinygltf::BufferView inverseBindPoseView;
	inverseBindPoseView.buffer = inverseBindPoseBufferIndex;
	inverseBindPoseView.byteOffset = 0;
	inverseBindPoseView.byteLength = meshData.bones.size() * sizeof(glm::mat4);
	const int inverseBindPoseBufferViewIndex = static_cast<int>(model.bufferViews.size());
	model.bufferViews.emplace_back(std::move(inverseBindPoseView));

	// Calculate min and max values for position attribute
	glm::vec3 minPos(std::numeric_limits<float>::max());
	glm::vec3 maxPos(std::numeric_limits<float>::min());
	for (const auto& vertex : meshData.vertices) {
		minPos = glm::min(minPos, vertex.pos);
		maxPos = glm::max(maxPos, vertex.pos);
	}

	// Vertex Position accessor
	tinygltf::Accessor posAccessor;
	posAccessor.bufferView = vertexBufferViewIndex;
	posAccessor.byteOffset = offsetof(Vertex, pos);
	posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	posAccessor.count = meshData.vertices.size();
	posAccessor.type = TINYGLTF_TYPE_VEC3;
	posAccessor.minValues = { minPos.x, minPos.y, minPos.z };
	posAccessor.maxValues = { maxPos.x, maxPos.y, maxPos.z };
	const int posAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(posAccessor));

	// Vertex Normal accessor
	tinygltf::Accessor normalAccessor;
	normalAccessor.bufferView = vertexBufferViewIndex;
	normalAccessor.byteOffset = offsetof(Vertex, normal);
	normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	normalAccessor.count = meshData.vertices.size();
	normalAccessor.type = TINYGLTF_TYPE_VEC3;
	const int normalAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(normalAccessor));

	// Vertex Tangent accessor
	tinygltf::Accessor tangentAccessor;
	tangentAccessor.bufferView = vertexBufferViewIndex;
	tangentAccessor.byteOffset = offsetof(Vertex, tangent);
	tangentAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	tangentAccessor.count = meshData.vertices.size();
	tangentAccessor.type = TINYGLTF_TYPE_VEC4;
	const int tangentAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(tangentAccessor));

	// Vertex Texture Coordinates accessor
	tinygltf::Accessor texCoordsAccessor;
	texCoordsAccessor.bufferView = vertexBufferViewIndex;
	texCoordsAccessor.byteOffset = offsetof(Vertex, texCoords);
	texCoordsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	texCoordsAccessor.count = meshData.vertices.size();
	texCoordsAccessor.type = TINYGLTF_TYPE_VEC2;
	const int texCoordsAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(texCoordsAccessor));

	// Vertex Joint Indices accessor
	tinygltf::Accessor jointIndicesAccessor;
	jointIndicesAccessor.bufferView = vertexBufferViewIndex;
	jointIndicesAccessor.byteOffset = offsetof(Vertex, jointIndices);
	jointIndicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
	jointIndicesAccessor.count = meshData.vertices.size();
	jointIndicesAccessor.type = TINYGLTF_TYPE_VEC4;
	const int jointIndicesAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(jointIndicesAccessor));

	// Vertex Joint Weights accessor
	tinygltf::Accessor jointWeightsAccessor;
	jointWeightsAccessor.bufferView = vertexBufferViewIndex;
	jointWeightsAccessor.byteOffset = offsetof(Vertex, jointWeights);
	jointWeightsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	jointWeightsAccessor.count = meshData.vertices.size();
	jointWeightsAccessor.type = TINYGLTF_TYPE_VEC4;
	const int jointWeightsAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(jointWeightsAccessor));

	// Index accessor
	tinygltf::Accessor indexAccessor;
	indexAccessor.bufferView = indexBufferViewIndex;
	indexAccessor.byteOffset = 0;
	indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	indexAccessor.count = meshData.indices.size();
	indexAccessor.type = TINYGLTF_TYPE_SCALAR;
	const int indexAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(indexAccessor));

	// Inverse bind pose accessor
	tinygltf::Accessor inverseBindPoseAccessor;
	inverseBindPoseAccessor.bufferView = inverseBindPoseBufferViewIndex;
	inverseBindPoseAccessor.byteOffset = 0;
	inverseBindPoseAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	inverseBindPoseAccessor.count = meshData.bones.size();
	inverseBindPoseAccessor.type = TINYGLTF_TYPE_MAT4;
	const int inverseBindPoseAccessorIndex = static_cast<int>(model.accessors.size());
	model.accessors.emplace_back(std::move(inverseBindPoseAccessor));

	// Parse diffuse texture
	ImageData diffuseImageData;
	if (!ImageData_Load(diffuseImageData, diffuseTextureFilename)) {
		LOG_ERROR("Failed to load diffuse texture: {}", diffuseTextureFilename);
		return false;
	}

	// Diffuse Image
	tinygltf::Image diffuseImage;
	diffuseImage.width = diffuseImageData.width;
	diffuseImage.height = diffuseImageData.height;
	diffuseImage.component = diffuseImageData.channels;
	diffuseImage.bits = 8;
	diffuseImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
	diffuseImage.image = std::move(diffuseImageData.data);
	diffuseImage.mimeType = "image/png";
	model.images.emplace_back(std::move(diffuseImage));

	// Parse normal texture
	ImageData normalImageData;
	if (!ImageData_Load(normalImageData, normalTextureFilename)) {
		LOG_ERROR("Failed to load normal texture: {}", normalTextureFilename);
		return false;
	}

	// Normal Image
	tinygltf::Image normalImage;
	normalImage.width = normalImageData.width;
	normalImage.height = normalImageData.height;
	normalImage.component = normalImageData.channels;
	normalImage.bits = 8;
	normalImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
	normalImage.image = std::move(normalImageData.data);
	normalImage.mimeType = "image/png";
	model.images.emplace_back(std::move(normalImage));

	// Sampler
	tinygltf::Sampler sampler;
	sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
	sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
	sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
	sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	model.samplers.emplace_back(std::move(sampler));

	// Diffuse Texture
	tinygltf::Texture diffuseTexture;
	diffuseTexture.source = 0; // image index
	diffuseTexture.sampler = 0; // sampler index
	model.textures.emplace_back(std::move(diffuseTexture));

	// Normal Texture
	tinygltf::Texture normalTexture;
	normalTexture.source = 1; // image index
	normalTexture.sampler = 0; // sampler index
	model.textures.emplace_back(std::move(normalTexture));

	// Material
	tinygltf::Material material;
	material.pbrMetallicRoughness.baseColorTexture.index = 0; // texture index
	material.pbrMetallicRoughness.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.pbrMetallicRoughness.metallicFactor = 0.0f;
	material.pbrMetallicRoughness.roughnessFactor = 1.0f;
	material.normalTexture.index = 1; // normal texture index
	model.materials.emplace_back(std::move(material));

	// Skin
	tinygltf::Skin skin;
	skin.inverseBindMatrices = inverseBindPoseAccessorIndex;
	skin.joints.resize(meshData.bones.size());
	for (size_t i = 0; i < meshData.bones.size(); ++i) {
		skin.joints[i] = static_cast<int>(i); // joint node indices
	}
	skin.skeleton = 0; // root node index
	model.skins.emplace_back(std::move(skin));

	// Primitive
	tinygltf::Primitive primitive;
	primitive.attributes["POSITION"] = posAccessorIndex;
	primitive.attributes["NORMAL"] = normalAccessorIndex;
	primitive.attributes["TANGENT"] = tangentAccessorIndex;
	primitive.attributes["TEXCOORD_0"] = texCoordsAccessorIndex;
	primitive.attributes["JOINTS_0"] = jointIndicesAccessorIndex;
	primitive.attributes["WEIGHTS_0"] = jointWeightsAccessorIndex;
	primitive.indices = indexAccessorIndex;
	primitive.material = 0; // material index
	primitive.mode = TINYGLTF_MODE_TRIANGLES;

	// Mesh
	tinygltf::Mesh mesh;
	mesh.primitives.emplace_back(std::move(primitive));
	model.meshes.emplace_back(std::move(mesh));

	// Joint Nodes
	for (size_t i = 0; i < meshData.bones.size(); ++i) {
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		DecomposeMatrix(meshData.bones[i].localBindPose, translation, rotation, scale);

		tinygltf::Node jointNode;
		jointNode.translation = { translation.x, translation.y, translation.z };
		jointNode.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
		jointNode.scale = { scale.x, scale.y, scale.z };
		jointNode.children.reserve(meshData.bones[i].childIndices.size());
		for (uint32_t childIndex : meshData.bones[i].childIndices) {
			jointNode.children.emplace_back(static_cast<int>(childIndex)); // child node indices
		}
		model.nodes.emplace_back(std::move(jointNode));
	}

	// Mesh Node (child of root joint)
	tinygltf::Node meshNode;
	meshNode.mesh = 0; // mesh index
	meshNode.skin = 0; // skin index
	const int meshNodeIndex = static_cast<int>(model.nodes.size());
	model.nodes.emplace_back(std::move(meshNode));

	// Scene
	tinygltf::Scene scene;
	scene.nodes.emplace_back(0); // root joint node index
	scene.nodes.emplace_back(meshNodeIndex); // mesh node index
	model.scenes.emplace_back(std::move(scene));
	model.defaultScene = 0;

	LOG_INFO("Saving mesh to GLTF file: {}", filename);

	// Save the model to a GLTF file
	tinygltf::TinyGLTF gltfContext;
	if (!gltfContext.WriteGltfSceneToFile(&model, filename, true, true, true, false)) {
		LOG_ERROR("Failed to save GLTF file: {}", filename);
		return false;
	}

	return true;
}

static bool ImageData_Load(ImageData& imageData, const char* filename) {
	int width = 0, height = 0, channels = 0;
	stbi_uc* imageDataRaw = stbi_load(filename, &width, &height, &channels, 4);
	if (!imageDataRaw) {
		LOG_ERROR("Failed to load texture image: {}", filename);
		return false;
	}

	std::vector<unsigned char> rgbaData(imageDataRaw, imageDataRaw + (width * height * 4));
	stbi_image_free(imageDataRaw);

	imageData.width = width;
	imageData.height = height;
	imageData.channels = 4;
	imageData.data = std::move(rgbaData);

	return true;
}

static void VertexInfluence_SortAndNormalize(std::vector<VertexInfluence>& influences) {
	// 1. Remove zero/near-zero weights
	constexpr float EPS = 1e-6f;
	influences.erase(
		std::remove_if(influences.begin(), influences.end(),
			[EPS](const VertexInfluence& i) { return i.weight < EPS; }),
		influences.end()
	);

	// 2. Sort descending by weight
	std::sort(influences.begin(), influences.end(),
		[](const VertexInfluence& a, const VertexInfluence& b) {
			return a.weight > b.weight;
		});

	// 3. Keep top 4
	influences.resize(4);

	// 4. Renormalize
	float sum = 0.0f;
	for (const auto& i : influences)
		sum += i.weight;

	if (sum > 0.0f) {
		for (auto& i : influences)
			i.weight /= sum;
	} else {
		// fallback: assign full weight to first bone
		influences.clear();
		influences.push_back({0, 1.0f});
	}
}

static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& outTranslation,
	glm::quat& outRotation, glm::vec3& outScale
) {
	outTranslation = glm::vec3(matrix[3]);

	const glm::vec3 col0 = glm::vec3(matrix[0]);
	const glm::vec3 col1 = glm::vec3(matrix[1]);
	const glm::vec3 col2 = glm::vec3(matrix[2]);

	outScale.x = glm::length(col0);
	outScale.y = glm::length(col1);
	outScale.z = glm::length(col2);

	glm::vec3 rot0 = col0 / outScale.x;
	const glm::vec3 rot1 = col1 / outScale.y;
	const glm::vec3 rot2 = col2 / outScale.z;

	if (glm::dot(glm::cross(rot0, rot1), rot2) < 0.0f) {
		outScale.x = -outScale.x;
		rot0 = -rot0;
	}

	glm::mat3 rotMat;
	rotMat[0] = rot0;
	rotMat[1] = rot1;
	rotMat[2] = rot2;

	outRotation = glm::normalize(glm::quat_cast(rotMat));
}
