
#include "Utils/Log.h"

#include "tiny_gltf.h"
#include "MeshData.h"

int main(int argc, char** argv) {
	Log::Init();

	if (argc < 2) {
		LOG_ERROR("No mesh file specified");
		LOG_ERROR("Usage: {} <mesh_file>", argv[0]);
		return -1;
	}

	MeshData meshData;
	if (!MeshData_LoadFromSfjFile(meshData, argv[1])) {
		LOG_ERROR("Failed to load mesh data");
		return -1;
	}

	return 0;
}
