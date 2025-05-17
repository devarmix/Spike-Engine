#include <Engine/Renderer/Material.h>
#include <Platforms/Vulkan/VulkanMaterial.h>
using namespace Spike;

namespace SpikeEngine {

	Ref<Material> Material::Create() {

		return VulkanMaterial::Create();
	}
}