#pragma once

#include <Engine/Renderer/RenderResource.h>
#include <Engine/Core/Core.h>

namespace Spike {

	enum ETextureFormat {

		EFormatNone = 0,
		EFormatRGBA8U,
		EFormatRGBA16F,
		EFormatRGBA32F,
		EFormatD32F,
		EFormatR32F
	};

	enum ETextureUsageFlagsBits {

		EUsageFlagSampled         = BIT(0),
		EUsageFlagTransferSrc     = BIT(1),
		EUsageFlagTransferDst     = BIT(2),
		EUsageFlagStorage         = BIT(3),
		EUsageFlagColorAttachment = BIT(4),
		EUsageFlagDepthAtachment  = BIT(5)
	};

	typedef uint32_t ETextureUsageFlags;

	class TextureResource : public RenderResource {
	public:

		TextureResource(ETextureFormat format, ETextureUsageFlags usageFlags)
			: m_Format(format), m_UsageFlags(usageFlags), m_GPUData(nullptr) {}

		ResourceGPUData* GetGPUData() { return m_GPUData; }

		ETextureFormat GetFormat() const { return m_Format; }
		ETextureUsageFlags GetUsageFlags() const { return m_UsageFlags; }

	protected:

		ResourceGPUData* m_GPUData;

		ETextureFormat m_Format;
		ETextureUsageFlags m_UsageFlags;
	};
}