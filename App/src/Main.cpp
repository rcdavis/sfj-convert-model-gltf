
#include "Utils/Log.h"

#include "tiny_gltf.h"
#include "MeshData.h"

int main(int argc, char** argv) {
	Log::Init();

	if (argc < 4) {
		LOG_ERROR("No mesh file, GLTF file, or diffuse texture file specified");
		LOG_ERROR("Usage: {} <mesh_file> <gltf_file> <diffuse_texture_file>", argv[0]);
		return -1;
	}

	MeshData meshData;
	if (!MeshData_LoadFromSfjFile(meshData, argv[1])) {
		LOG_ERROR("Failed to load mesh data");
		return -1;
	}

	if (!MeshData_SaveToGltfFile(meshData, argv[2], argv[3])) {
		LOG_ERROR("Failed to save mesh data to GLTF file");
		return -1;
	}

	return 0;
}
