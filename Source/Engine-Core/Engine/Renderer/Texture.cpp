#include <Engine/Renderer/Texture.h>
#include <Platforms/Vulkan/VulkanTexture.h>
using namespace Spike;

namespace SpikeEngine {

	Ref<Texture> Texture::Create(const char* filePath) {

		return VulkanTexture::Create(filePath);
	}
}