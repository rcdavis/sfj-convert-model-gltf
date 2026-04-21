
#include "Utils/Log.h"

#include "tiny_gltf.h"

int main(int argc, char** argv) {
	Log::Init();

	LOG_INFO("Hello, World!");

	return 0;
}
