#include <Engine/Utils/RenderUtils.h>
#include <fstream>

uint8_t* Spike::RenderUtils::LoadSMAA_AreaTex() {

	std::ifstream file("C:/Users/Artem/Desktop/Spike-Engine/Resources/Renderer/SMAA_AreaTex.bin", std::ios::binary);
	const size_t texSize = 179200;

	uint8_t* out = new uint8_t[texSize];
	file.read((char*)out, texSize);
	file.close();

	return out;
}

uint8_t* Spike::RenderUtils::LoadSMAA_SearchTex() {

	std::ifstream file("C:/Users/Artem/Desktop/Spike-Engine/Resources/Renderer/SMAA_SearchTex.bin", std::ios::binary);
	const size_t texSize = 1024;

	uint8_t* out = new uint8_t[texSize];
	file.read((char*)out, texSize);
	file.close();

	return out;
}