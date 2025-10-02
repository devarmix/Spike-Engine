#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Core/Log.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>

Spike::FrameRenderer* Spike::GFrameRenderer = nullptr;

namespace Spike {

	void FrameRenderer::FlushFrameQueue(uint32_t idx) {

		std::vector<std::function<void()>> queue{};
		m_FrameQueues[idx].PopAll(queue);

		for (auto& func : queue) {
			func();
		}
	}

	FrameRenderer::FrameRenderer() : 
		m_BRDFLut(nullptr), m_GuiFontTexture(nullptr),
		m_CommandBuffers{nullptr, nullptr} 
	{
		if (GApplication->IsUsingImGui()) {
			ImGui::CreateContext();

			// TODO: change in future
			ImGui_ImplSDL2_InitForVulkan((SDL_Window*)GApplication->GetMainWindow()->GetNativeWindow());

			if (GApplication->IsUsingDocking()) {
				ImGuiIO& io = ImGui::GetIO();
				io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			}
		}

		SUBMIT_RENDER_COMMAND([this]() {

			for (int i = 0; i < 2; i++) {
				m_CommandBuffers[i] = new RHICommandBuffer();
				m_CommandBuffers[i]->InitRHI();
			}

			// generate brdf lut
			{
				SamplerDesc brdfSamplerDesc{};
				brdfSamplerDesc.Filter = ESamplerFilter::EBilinear;
				brdfSamplerDesc.AddressU = ESamplerAddress::EClamp;
				brdfSamplerDesc.AddressV = ESamplerAddress::EClamp;
				brdfSamplerDesc.AddressW = ESamplerAddress::EClamp;

				constexpr uint32_t BRDF_LUT_SIZE = 256;

				Texture2DDesc brdfDesc{};
				brdfDesc.Width = BRDF_LUT_SIZE;
				brdfDesc.Height = BRDF_LUT_SIZE;
				brdfDesc.Format = ETextureFormat::ERG16F;
				brdfDesc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ESampled;
				brdfDesc.NumMips = 1;
				brdfDesc.SamplerDesc = brdfSamplerDesc;

				m_BRDFLut = new RHITexture2D(brdfDesc);
				m_BRDFLut->InitRHI();

				ShaderDesc shaderDesc{};
				shaderDesc.Type = EShaderType::ECompute;
				shaderDesc.Name = "BRDF";

				RHIShader* shader = GShaderManager->GetShaderFromCache(shaderDesc);

				RHIBindingSet* brdfSet = new RHIBindingSet(shader->GetLayouts()[0]);
				brdfSet->InitRHI();
				brdfSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureUAV, m_BRDFLut->GetTextureView(), EGPUAccessFlags::EUAVCompute);

				GRHIDevice->ImmediateSubmit([&, this](RHICommandBuffer* cmd) {

					GRHIDevice->BarrierTexture(cmd, m_BRDFLut, EGPUAccessFlags::ENone, EGPUAccessFlags::EUAVCompute);
					GRHIDevice->BindShader(cmd, shader, { brdfSet });

					uint32_t groupCount = GetComputeGroupCount(BRDF_LUT_SIZE, 8);
					GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

					GRHIDevice->BarrierTexture(cmd, m_BRDFLut, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ESRV);
					});

				brdfSet->ReleaseRHI();
				delete brdfSet;
			}
			});

		m_FrameCount = 0;
	}

	FrameRenderer::~FrameRenderer() {

		for (int i = 0; i < 2; i++) {

			FlushFrameQueue(i);
			m_CommandBuffers[i]->ReleaseRHIImmediate();
			delete m_CommandBuffers[i];
		}

		m_BRDFLut->ReleaseRHIImmediate();
		delete m_BRDFLut;

		if (m_GuiFontTexture) {
			m_GuiFontTexture->ReleaseRHIImmediate();
			delete m_GuiFontTexture;
		}

		if (GApplication->IsUsingImGui()) {
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
		}
	}

	void FrameRenderer::RenderWorld(RHIWorldProxy* proxy, RenderContext context, const CameraDrawData& cameraData, std::vector<RenderFeature*>& features) {

		RDGBuilder builder = RDGBuilder();
		uint32_t frameIndex = m_FrameCount % 2;

		// reset draw counts buffer
		GRHIDevice->FillBuffer(m_CommandBuffers[frameIndex], proxy->DrawCountsBuffer, proxy->DrawCountsBuffer->GetSize(), 0, 0, EGPUAccessFlags::EIndirectArgs, EGPUAccessFlags::EUAVCompute);

		for (auto& feature : features) {
			feature->BuildGraph(&builder, proxy, context, &cameraData);
		}

		builder.Execute(m_CommandBuffers[frameIndex]);
	}

	void FrameRenderer::RenderSwapchain(uint32_t width, uint32_t height, RHITexture2D* fillTexture) {

		RHIDevice::ImGuiRTState* guiState = nullptr;
		if (GApplication->IsUsingImGui()) {
			ImGui::EndFrame();
			ImGui::Render();

			ImDrawData* drawData = ImGui::GetDrawData();
			guiState = &m_GuiRTStates[m_FrameCount % 2];

			// perform an imgui rt state copy
			{
				guiState->TotalIdxCount = drawData->TotalIdxCount;
				guiState->TotalVtxCount = drawData->TotalVtxCount;
				guiState->DisplayPos = drawData->DisplayPos;
				guiState->DisplaySize = drawData->DisplaySize;
				guiState->FramebufferScale = drawData->FramebufferScale;
				guiState->CmdLists.clear();
				guiState->CmdLists.resize(drawData->CmdListsCount);

				for (int l = 0; l < drawData->CmdListsCount; l++) {

					ImDrawList* list = drawData->CmdLists[l];
					RHIDevice::ImGuiRTState::DrawList& rtList = guiState->CmdLists[l];

					rtList.CmdBuffer.reserve(list->CmdBuffer.Size);
					for (int cmd = 0; cmd < list->CmdBuffer.Size; cmd++) {

						const ImDrawCmd* pcmd = &list->CmdBuffer[cmd];
						if (pcmd->UserCallback != nullptr) {
							pcmd->UserCallback(list, pcmd);
						}
						else {
							rtList.CmdBuffer.push_back(*pcmd);
						}
					}

					list->VtxBuffer.swap(rtList.VtxBuffer);
					list->IdxBuffer.swap(rtList.IdxBuffer);
				}
			}
		}

		SUBMIT_RENDER_COMMAND(([=, this]() {
			GRHIDevice->DrawSwapchain(m_CommandBuffers[m_FrameCount % 2], width, height, guiState, fillTexture);
			}));

		GApplication->EnqueueEvent([this]() {
			m_FrameCount++;
			});
	}

	void FrameRenderer::UpdateFontTexture(uint8_t** outData) {
		ImGuiIO& io = ImGui::GetIO();

		if (m_GuiFontTexture && io.Fonts->TexID)
			return;

		int width, height;
		io.Fonts->GetTexDataAsRGBA32(outData, &width, &height);
		if (!*outData)
			return;

		SamplerDesc samplDesc{};
		samplDesc.Filter = ESamplerFilter::ETrilinear;
		samplDesc.AddressU = ESamplerAddress::EClamp;
		samplDesc.AddressV = ESamplerAddress::EClamp;
		samplDesc.AddressW = ESamplerAddress::EClamp;

		Texture2DDesc desc{};
		desc.Width = width;
		desc.Height = height;
		desc.NumMips = 1;
		desc.Format = ETextureFormat::ERGBA8U;
		desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
		desc.SamplerDesc = samplDesc;

		m_GuiFontTexture = new RHITexture2D(desc);
		io.Fonts->SetTexID((ImTextureID)m_GuiFontTexture);
		io.Fonts->TexReady = true;
	}

	void FrameRenderer::BeginFrame() {
		
		RHITexture2D* oldFontsTex = m_GuiFontTexture;
		uint8_t* newFontsData = nullptr;

		if (GApplication->IsUsingImGui()) {
			UpdateFontTexture(&newFontsData);

			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
		}

		SUBMIT_RENDER_COMMAND(([=, this]() {
			RHICommandBuffer* cmd = m_CommandBuffers[m_FrameCount % 2]; 
			GRHIDevice->WaitForFrameCommandBuffer(cmd);
			 
			FlushFrameQueue(m_FrameCount % 2);
			GRHIDevice->BeginFrameCommandBuffer(cmd); 

			if (newFontsData) {
				if (oldFontsTex) {
					oldFontsTex->ReleaseRHI();
					delete oldFontsTex;
				}

				RHIDevice::SubResourceCopyRegion region{ 0, 0, 0 };
				m_GuiFontTexture->InitRHI();
				GRHIDevice->CopyDataToTexture(newFontsData, 0, m_GuiFontTexture, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, { region }, 
					(size_t)m_GuiFontTexture->GetSizeXYZ().x * m_GuiFontTexture->GetSizeXYZ().y * TextureFormatToSize(m_GuiFontTexture->GetFormat()));
			}
			}));
	}
}