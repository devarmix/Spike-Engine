#pragma once

#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/Shader.h>

#define INVALID_RDG_HANDLE UINT16_MAX

namespace Spike {

	enum class ERendererStage : uint8_t {

		EBeforeRender = 0,
		EOpaqueRender,
		EAfterOpaqueRender,
		ELightsRender,
		EAfterLightsRender,
		ESkyboxRender,
		EAfterSkyboxRender,
		EPostProcessingRender,
		EAfterPostProcessingRender,
		EAfterRender
	};

	using RDGHandle = uint16_t;

	class RHICommandBuffer;

	class RDGResourcePool {
	public:
		RDGResourcePool() {}
		~RDGResourcePool() { FreeAll(); }

		void FreeUnused();
		void FreeAll();

		RHITexture2D* GetOrCreateTexture2D(const Texture2DDesc& desc);
		RHITextureView* GetOrCreateTextureView(const TextureViewDesc& desc);
		RHIBuffer* GetOrCreateBuffer(const BufferDesc& desc);
		RHIBindingSet* GetOrCreateBindingSet(RHIBindingSetLayout* layout);

	private:

		struct RDGPooledTexture {

			uint32_t LastUsedFrame;
			RHITexture2D* Resource;
		};

		struct RDGPooledTextureView {

			uint32_t LastUsedFrame;
			RHITextureView* Resource;
		};

		struct RDGPooledBuffer {

			uint32_t LastUsedFrame;
			RHIBuffer* Resource;
		};

		struct RDGPooledBindingSet {

			uint32_t LastUsedFrame;
			RHIBindingSet* Resource;
		};

		std::vector<RDGPooledTexture> m_TexturePool;
		std::vector<RDGPooledTextureView> m_TextureViewPool;
		std::vector<RDGPooledBuffer> m_BufferPool;
		std::vector<RDGPooledBindingSet> m_SetPool;

		const uint32_t m_FramesBeforeDelete = 2;
	};

	// global rdg pool pointer
	extern RDGResourcePool* GRDGPool;

	class RDGBuilder {
	public:
		RDGBuilder() {}
		~RDGBuilder() {}

		template<typename LambdaFunc>
		void AddPass(const std::vector<RDGHandle>& usedRDGTextures, const std::vector<RDGHandle>& usedRDGBuffers, ERendererStage rendererStage, LambdaFunc&& passLambda) {

			// some value we would never ever reach
			const uint16_t maxPassesPerStage = 1000;

			uint8_t stageIndex = (uint8_t)rendererStage;
			uint32_t passHandle = uint32_t((stageIndex * maxPassesPerStage) + m_Passes[stageIndex].size());

			for (auto handle : usedRDGTextures) {

				m_Textures[handle].FirstPassUse = std::min(m_Textures[handle].FirstPassUse, passHandle);
				m_Textures[handle].LastPassUse = std::max(m_Textures[handle].LastPassUse, passHandle);
			}

			for (auto handle : usedRDGBuffers) {

				m_Buffers[handle].FirstPassUse = std::min(m_Buffers[handle].FirstPassUse, passHandle);
				m_Buffers[handle].LastPassUse = std::max(m_Buffers[handle].LastPassUse, passHandle);
			}

			m_Passes[stageIndex].push_back(RDGPass{.Func = std::forward<LambdaFunc>(passLambda), .UsedTextures = usedRDGTextures, .UsedBuffers = usedRDGBuffers});
		}

		RDGHandle CreateRDGTexture2D(const std::string& name, const Texture2DDesc& desc);
		RDGHandle CreateRDGBuffer(const std::string& name, const BufferDesc& desc);

		void RegisterExternalTexture2D(RHITexture2D* tex, EGPUAccessFlags currentAccess = EGPUAccessFlags::ENone);
		void RegisterExternalBuffer(RHIBuffer* buff, EGPUAccessFlags currentAccess = EGPUAccessFlags::ENone);

		RDGHandle FindRDGTexture2D(const std::string& name);
		RDGHandle FindRDGBuffer(const std::string& name);
		RHITexture2D* GetTextureResource(RDGHandle handle);
		RHIBuffer* GetBufferResource(RDGHandle handle);

		void BarrierRDGTexture2D(RHICommandBuffer* cmd, RHITexture2D* tex, EGPUAccessFlags newAccess);
		void BarrierRDGBuffer(RHICommandBuffer* cmd, RHIBuffer* buff, size_t size, size_t offset, EGPUAccessFlags newAccess);
		void FillRDGBuffer(RHICommandBuffer* cmd, RHIBuffer* buff, size_t size, size_t offset, uint32_t value, EGPUAccessFlags newAccess);

		void Execute(RHICommandBuffer* cmd);

	private:

		struct RDGResource {

			uint32_t FirstPassUse = 0;
			uint32_t LastPassUse = 0;

			std::string Name;
		};

		struct RDGTexture : public RDGResource {

			Texture2DDesc Desc;
			RHITexture2D* Resource = nullptr;
		};

		struct RDGBuffer : public RDGResource {

			BufferDesc Desc;
			RHIBuffer* Resource = nullptr;
		};

		struct RDGPass {

			std::function<void(RHICommandBuffer*)> Func;

			std::vector<RDGHandle> UsedTextures;
			std::vector<RDGHandle> UsedBuffers;
		};

		std::vector<RDGPass> m_Passes[10];

		std::vector<RDGTexture> m_Textures;
		std::vector<RDGBuffer> m_Buffers;

		std::unordered_map<RHITexture2D*, EGPUAccessFlags> m_TextureAccessMap;
		std::unordered_map<RHIBuffer*, EGPUAccessFlags> m_BufferAccessMap;
	};
}