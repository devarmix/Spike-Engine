#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Log.h>

Spike::RDGResourcePool* Spike::GRDGPool = nullptr;

namespace Spike {

	void RDGResourcePool::FreeUnused() {

		auto swapDelete = [](auto& v, size_t el) {

			if (v.size() > 1) {

				auto& last = v.back();
				v[el] = last;
			}

			v.pop_back();
			};

		uint32_t viewIndex = 0;
		while (viewIndex < m_TextureViewPool.size()) {

			if (m_TextureViewPool[viewIndex].LastUsedFrame + m_FramesBeforeDelete < GFrameRenderer->GetFrameCount()) {

				m_TextureViewPool[viewIndex].Resource->ReleaseRHI();
				delete m_TextureViewPool[viewIndex].Resource;

				swapDelete(m_TextureViewPool, viewIndex);
			}
			else {

				viewIndex++;
			}
		}

		// debug TODO: Move or remove
		uint32_t allocTexSize = 0;
		uint32_t texIndex = 0;
		while (texIndex < m_TexturePool.size()) {

			allocTexSize += m_TexturePool[texIndex].Resource->GetSizeXYZ().x * m_TexturePool[texIndex].Resource->GetSizeXYZ().y * TextureFormatToSize(m_TexturePool[texIndex].Resource->GetFormat());

			if (m_TexturePool[texIndex].LastUsedFrame + m_FramesBeforeDelete < GFrameRenderer->GetFrameCount()) {

				m_TexturePool[texIndex].Resource->ReleaseRHI();
				delete m_TexturePool[texIndex].Resource;

				swapDelete(m_TexturePool, texIndex);
			}
			else {

				texIndex++;
			}
		}
		ENGINE_TRACE("RDGResourcePool allocate textures size: {} MB", allocTexSize / 1000000);

		uint32_t buffIndex = 0;
		while (buffIndex < m_BufferPool.size()) {

			if (m_BufferPool[buffIndex].LastUsedFrame + m_FramesBeforeDelete < GFrameRenderer->GetFrameCount()) {

				m_BufferPool[buffIndex].Resource->ReleaseRHI();
				delete m_BufferPool[buffIndex].Resource;

				swapDelete(m_BufferPool, buffIndex);
			}
			else {

				buffIndex++;
			}
		}

		uint32_t setIndex = 0;
		while (setIndex < m_SetPool.size()) {

			if (m_SetPool[setIndex].LastUsedFrame + m_FramesBeforeDelete < GFrameRenderer->GetFrameCount()) {

				m_SetPool[setIndex].Resource->ReleaseRHI();
				delete m_SetPool[setIndex].Resource;

				swapDelete(m_SetPool, setIndex);
			}
			else {

				setIndex++;
			}
		}
	}

	void RDGResourcePool::FreeAll() {

		for (auto& view : m_TextureViewPool) {

			view.Resource->ReleaseRHIImmediate();
			delete view.Resource;
		}

		for (auto& tex : m_TexturePool) {

			tex.Resource->ReleaseRHIImmediate();
			delete tex.Resource;
		}

		for (auto& buff : m_BufferPool) {

			buff.Resource->ReleaseRHIImmediate();
			delete buff.Resource;
		}

		for (auto& set : m_SetPool) {

			set.Resource->ReleaseRHIImmediate();
			delete set.Resource;
		}

		m_TexturePool.clear();
		m_TextureViewPool.clear();
		m_BufferPool.clear();
		m_SetPool.clear();
	}

	RHITexture2D* RDGResourcePool::GetOrCreateTexture2D(const Texture2DDesc& desc) {

		auto it = std::find_if(m_TexturePool.begin(), m_TexturePool.end(), [&desc](const auto& e) {
			return e.Resource->GetDesc() == desc && e.LastUsedFrame < GFrameRenderer->GetFrameCount();
			});

		if (it != m_TexturePool.end()) {

			it->LastUsedFrame = GFrameRenderer->GetFrameCount();
			return it->Resource;
		}
		else {

			RHITexture2D* res = new RHITexture2D(desc);
			res->InitRHI();

			RDGPooledTexture newPooled{.LastUsedFrame = GFrameRenderer->GetFrameCount(), .Resource = res};
			m_TexturePool.push_back(newPooled);

			return res;
		}
	}

	RHITextureView* RDGResourcePool::GetOrCreateTextureView(const TextureViewDesc& desc) {

		auto it = std::find_if(m_TextureViewPool.begin(), m_TextureViewPool.end(), [&desc](const auto& e) {
			return e.Resource->GetDesc() == desc;
			});

		if (it != m_TextureViewPool.end()) {

			it->LastUsedFrame = GFrameRenderer->GetFrameCount();
			return it->Resource;
		}
		else {

			RHITextureView* res = new RHITextureView(desc);
			res->InitRHI();

			RDGPooledTextureView newPooled{ .LastUsedFrame = GFrameRenderer->GetFrameCount(), .Resource = res };
			m_TextureViewPool.push_back(newPooled);

			return res;
		}
	}

	RHIBuffer* RDGResourcePool::GetOrCreateBuffer(const BufferDesc& desc) {

		auto it = std::find_if(m_BufferPool.begin(), m_BufferPool.end(), [&desc](const auto& e) {
			return e.Resource->GetDesc() == desc && e.LastUsedFrame + 1 < GFrameRenderer->GetFrameCount();
			});

		if (it != m_BufferPool.end()) {

			it->LastUsedFrame = GFrameRenderer->GetFrameCount();
			return it->Resource;
		}
		else {

			RHIBuffer* res = new RHIBuffer(desc);
			res->InitRHI();

			RDGPooledBuffer newPooled{ .LastUsedFrame = GFrameRenderer->GetFrameCount(), .Resource = res };
			m_BufferPool.push_back(newPooled);

			return res;
		}
	}

	RHIBindingSet* RDGResourcePool::GetOrCreateBindingSet(RHIBindingSetLayout* layout) {

		auto it = std::find_if(m_SetPool.begin(), m_SetPool.end(), [layout](const auto& e) {
			return e.Resource->GetLayout() == layout && e.LastUsedFrame + 1 < GFrameRenderer->GetFrameCount();
			});

		if (it != m_SetPool.end()) {

			it->LastUsedFrame = GFrameRenderer->GetFrameCount();
			return it->Resource;
		}
		else {

			RHIBindingSet* res = new RHIBindingSet(layout);
			res->InitRHI();

			RDGPooledBindingSet newPooled{ .LastUsedFrame = GFrameRenderer->GetFrameCount(), .Resource = res };
			m_SetPool.push_back(newPooled);

			return res;
		}
	}

	RDGHandle RDGBuilder::CreateRDGTexture2D(const std::string& name, const Texture2DDesc& desc) {

		RDGHandle handle = (RDGHandle)m_Textures.size();

		RDGTexture newTexture{};
		newTexture.Desc = desc;
		newTexture.Name = name;

		m_Textures.push_back(newTexture);

		return handle;
	}

	RDGHandle RDGBuilder::CreateRDGBuffer(const std::string& name, const BufferDesc& desc) {

		RDGHandle handle = (RDGHandle)m_Buffers.size();

		RDGBuffer newTexture{};
		newTexture.Desc = desc;
		newTexture.Name = name;

		m_Buffers.push_back(newTexture);

		return handle;
	}

	void RDGBuilder::RegisterExternalTexture2D(RHITexture2D* tex, EGPUAccessFlags currentAccess) {

		m_TextureAccessMap[tex] = currentAccess;
	}

	void RDGBuilder::RegisterExternalBuffer(RHIBuffer* buff, EGPUAccessFlags currentAccess) {

		m_BufferAccessMap[buff] = currentAccess;
	}

	void RDGBuilder::BarrierRDGTexture2D(RHICommandBuffer* cmd, RHITexture2D* tex, EGPUAccessFlags newAccess) {

		auto it = m_TextureAccessMap.find(tex);
		if (it != m_TextureAccessMap.end()) {

			GRHIDevice->BarrierTexture(cmd, tex, m_TextureAccessMap[tex], newAccess);
			m_TextureAccessMap[tex] = newAccess;
		}
		else {

			ENGINE_ERROR("Trying to barrier rdg texture that wasnt registered!");
		}
	}

	void RDGBuilder::BarrierRDGBuffer(RHICommandBuffer* cmd, RHIBuffer* buff, size_t size, size_t offset, EGPUAccessFlags newAccess) {

		auto it = m_BufferAccessMap.find(buff);
		if (it != m_BufferAccessMap.end()) {

			GRHIDevice->BarrierBuffer(cmd, buff, size, offset, m_BufferAccessMap[buff], newAccess);
			m_BufferAccessMap[buff] = newAccess;
		}
		else {

			ENGINE_ERROR("Trying to barrier rdg buffer that wasnt registered!");
		}
	}

	void RDGBuilder::FillRDGBuffer(RHICommandBuffer* cmd, RHIBuffer* buff, size_t size, size_t offset, uint32_t value, EGPUAccessFlags newAccess) {

		auto it = m_BufferAccessMap.find(buff);
		if (it != m_BufferAccessMap.end()) {

			GRHIDevice->FillBuffer(cmd, buff, size, offset, value, m_BufferAccessMap[buff], newAccess);
			m_BufferAccessMap[buff] = newAccess;
		}
		else {

			ENGINE_ERROR("Trying to fill rdg buffer that wasnt registered!");
		}
	}

	RDGHandle RDGBuilder::FindRDGTexture2D(const std::string& name) {

		auto it = std::find_if(m_Textures.begin(), m_Textures.end(), [&name](const auto& e) {
			return e.Name == name;
			});

		if (it != m_Textures.end()) {

			return (RDGHandle)std::distance(m_Textures.begin(), it);
		}
		else {

			ENGINE_ERROR("RDG texture with the name {} does not exist!", name);
			return INVALID_RDG_HANDLE;
		}
	}

	RDGHandle RDGBuilder::FindRDGBuffer(const std::string& name) {

		auto it = std::find_if(m_Buffers.begin(), m_Buffers.end(), [&name](const auto& e) {
			return e.Name == name;
			});

		if (it != m_Buffers.end()) {

			return (RDGHandle)std::distance(m_Buffers.begin(), it);
		}
		else {

			ENGINE_ERROR("RDG buffer with the name {} does not exist!", name);
			return INVALID_RDG_HANDLE;
		}
	}

	RHITexture2D* RDGBuilder::GetTextureResource(RDGHandle handle) {

		if (handle != INVALID_RDG_HANDLE) {

			return m_Textures[handle].Resource;
		}
		else {

			ENGINE_ERROR("Trying to access rdg texture with invalid handle! Undefined behaviour!");
			return nullptr;
		}
	}

	RHIBuffer* RDGBuilder::GetBufferResource(RDGHandle handle) {

		if (handle != INVALID_RDG_HANDLE) {

			return m_Buffers[handle].Resource;
		}
		else {

			ENGINE_ERROR("Trying to access rdg buffer with invalid handle! Undefined behaviour!");
			return nullptr;
		}
	}

	void RDGBuilder::Execute(RHICommandBuffer* cmd) {

		std::vector<RDGHandle> tempTexAliasedPool;
		std::vector<RDGHandle> tempBufferAliasedPool;

		for (int i = 0; i < 10; i++) {

			for (auto& pass : m_Passes[i]) {

				// resolve pass rdg resources
				{
					for (auto handle : pass.UsedTextures) {

						RDGTexture& tex = m_Textures[handle];
						if (tex.Resource) continue;

						for (auto aliasedHandle : tempTexAliasedPool) {

							if (m_Textures[aliasedHandle].LastPassUse < tex.FirstPassUse) {

								if (tex.Desc == m_Textures[aliasedHandle].Desc) {
									m_Textures[aliasedHandle].LastPassUse = tex.LastPassUse;
									tex.Resource = m_Textures[aliasedHandle].Resource;

									break;
								}
							}
						}

						// not found in aliased pool, create a new one and push to this pool
						if (!tex.Resource) {

							tex.Resource = GRDGPool->GetOrCreateTexture2D(tex.Desc);
							tempTexAliasedPool.push_back(handle);
						}

						m_TextureAccessMap[tex.Resource] = EGPUAccessFlags::ENone;
					}

					for (auto handle : pass.UsedBuffers) {

						RDGBuffer& buff = m_Buffers[handle];
						if (buff.Resource) continue;

						for (auto aliasedHandle : tempBufferAliasedPool) {

							if (m_Buffers[aliasedHandle].LastPassUse < buff.FirstPassUse) {

								if(buff.Desc == m_Buffers[aliasedHandle].Desc) {
									m_Buffers[aliasedHandle].LastPassUse = buff.LastPassUse;
									buff.Resource = m_Buffers[aliasedHandle].Resource;
								}

								break;
							}
						}

						// not found in aliased pool, create a new one and push to this pool
						if (!buff.Resource) {

							buff.Resource = GRDGPool->GetOrCreateBuffer(buff.Desc);
							tempBufferAliasedPool.push_back(handle);
						}

						m_BufferAccessMap[buff.Resource] = EGPUAccessFlags::ENone;
					}
				}

				// execute pass
				pass.Func(cmd);
			}
		}

		ENGINE_TRACE("RenderGraph virtual tex handles: {}", m_Textures.size());
		ENGINE_TRACE("RenderGraph aliased tex: {}", tempTexAliasedPool.size());
	}
}