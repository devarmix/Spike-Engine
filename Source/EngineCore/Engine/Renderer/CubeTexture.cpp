#include <Engine/Renderer/CubeTexture.h>
#include <Platforms/Vulkan/VulkanCubeTexture.h>
using namespace Spike;

namespace SpikeEngine {

	Ref<CubeTexture> CubeTexture::Create(const char** filePath) {

		return VulkanCubeTexture::Create(filePath);
	}
}