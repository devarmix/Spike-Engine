#include <Engine/Renderer/CubeTexture.h>
#include <Platforms/Vulkan/VulkanCubeTexture.h>
using namespace Spike;

namespace SpikeEngine {

	Ref<CubeTexture> CubeTexture::Create(const std::array<const char*, 6>& filePath) {

		return VulkanCubeTexture::Create(filePath);
	}
}