#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Renderer/RHIResource.h>

namespace Spike {

	enum class EBufferMemUsage {

		ENone = 0,
		ECPUToGPU,
		ECPUOnly,
		EGPUOnly
	};

	enum class EBufferUsageFlags : uint8_t {

		ENone = 0,
		EConstant     = BIT(0),
		EStorage      = BIT(1),
		ECopySrc      = BIT(2),
		ECopyDst      = BIT(3),
		EIndirect     = BIT(4),
		EAddressable  = BIT(5)
	};
	ENUM_FLAGS_OPERATORS(EBufferUsageFlags)

	struct BufferDesc {

		size_t Size;
		EBufferUsageFlags UsageFlags;
		EBufferMemUsage MemUsage;

		bool operator==(const BufferDesc& other) const {

			return (Size == other.Size
				&& MemUsage == other.MemUsage
				&& UsageFlags == other.UsageFlags);
		}
	};

	class RHIBuffer : public RHIResource {
	public:
		RHIBuffer(const BufferDesc& desc) : m_Desc(desc), m_RHIData(nullptr), m_MappedData(nullptr), m_GPUAddress(0) {}
	    virtual ~RHIBuffer() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		size_t GetSize() const { return m_Desc.Size; }
		EBufferUsageFlags GetUsageFlags() const { return m_Desc.UsageFlags; }
		EBufferMemUsage GetMemUsage() const { return m_Desc.MemUsage; }

		RHIData* GetRHIData() { return m_RHIData; }
		void* GetMappedData() { return m_MappedData; }

		const BufferDesc& GetDesc() { return m_Desc; }
		uint64_t GetGPUAddress() const { return m_GPUAddress; }

	private:

		void* m_MappedData;
		RHIData* m_RHIData;

		BufferDesc m_Desc;
		uint64_t m_GPUAddress;
	};
}

