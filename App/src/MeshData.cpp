#include "MeshData.h"

#include <fstream>
#include "Utils/Log.h"

bool MeshData_LoadFromFile(MeshData& meshData, const char* filename) {
	LOG_INFO("Loading mesh from file: {}", filename);

	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		LOG_ERROR("Failed to open file: {}", filename);
		return false;
	}


	return true;
}
