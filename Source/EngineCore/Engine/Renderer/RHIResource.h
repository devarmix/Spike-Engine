#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	enum class EGPUAccessFlags {

		ENone = 0,
		EIndirectArgs   = BIT(0),
		ESRVCompute     = BIT(1),
		ESRVGraphics    = BIT(2),
		EUAVCompute     = BIT(3),
		EUAVGraphics     = BIT(4),
		ECopySrc        = BIT(5),
		ECopyDst        = BIT(6),
		EColorTarget    = BIT(7),
		EDepthTarget    = BIT(8),

		ESRV = ESRVCompute | ESRVGraphics,
		EUAV = EUAVCompute | EUAVGraphics
	};
	ENUM_FLAGS_OPERATORS(EGPUAccessFlags)

	class RHIData {
	public:
		virtual ~RHIData() = default;

		virtual void* GetNativeData() = 0;
	};

	class RHIResource {
	public:
		virtual ~RHIResource() = default;

		// executed only by render thread!
		virtual void InitRHI() = 0;

		// executed only by render thread!
		virtual void ReleaseRHI() = 0;

		// executed only by render thread!
		virtual void ReleaseRHIImmediate() { ReleaseRHI(); }
	};

	void SafeRHIResourceInit(RHIResource* resource);
	void SafeRHIResourceRelease(RHIResource* resource);
}