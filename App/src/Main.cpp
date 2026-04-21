
#include "Utils/Log.h"

#include "tiny_gltf.h"
#include "MeshData.h"

int main(int argc, char** argv) {
	Log::Init();

	if (argc < 5) {
		LOG_ERROR("No mesh file, GLTF file, or texture files specified");
		LOG_ERROR("Usage: {} <mesh_file> <gltf_file> <diffuse_texture_file> <normal_texture_file>", argv[0]);
		return -1;
	}

	MeshData meshData;
	if (!MeshData_LoadFromSfjFile(meshData, argv[1])) {
		LOG_ERROR("Failed to load mesh data");
		return -1;
	}

	if (!MeshData_SaveToGltfFile(meshData, argv[2], argv[3], argv[4])) {
		LOG_ERROR("Failed to save mesh data to GLTF file");
		return -1;
	}

	return 0;
}
