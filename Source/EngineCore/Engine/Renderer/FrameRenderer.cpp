#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Generated/GeneratedBRDF.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Core/Log.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>

Spike::FrameRenderer* Spike::GFrameRenderer = nullptr;

namespace Spike {

	void FrameRenderer::FuncQueue::PushFunction(std::function<void()>&& function) {
		Queue.push_back(std::move(function));
	}

	void FrameRenderer::FuncQueue::Flush() {

		for (auto it = Queue.begin(); it != Queue.end(); it++) {
			(*it)();
		}

		Queue.clear();
	}

	FrameRenderer::FrameRenderer() {

		for (int i = 0; i < 2; i++) {

			{
				BufferDesc desc{};
				desc.Size = sizeof(SceneLightGPUData) * MAX_LIGHTS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage;
				desc.MemUsage = EBufferMemUsage::ECPUToGPU;

				m_FrameData[i].LightsBuffer = new RHIBuffer(desc);
				m_FrameData[i].LightsBuffer->InitRHI();
			}
			{
				BufferDesc desc{};
				desc.Size = sizeof(DrawIndirectCommand) * MAX_DRAWS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::EIndirect;
				desc.MemUsage = EBufferMemUsage::EGPUOnly;

				m_FrameData[i].DrawCommandsBuffer = new RHIBuffer(desc);
				m_FrameData[i].DrawCommandsBuffer->InitRHI();
			}
			{
				BufferDesc desc{};
				desc.Size = sizeof(uint32_t) * MAX_SHADERS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::EIndirect | EBufferUsageFlags::ECopyDst;
				desc.MemUsage = EBufferMemUsage::EGPUOnly;

				m_FrameData[i].DrawCountsBuffer = new RHIBuffer(desc);
				m_FrameData[i].DrawCountsBuffer->InitRHI();
			}
			{
				BufferDesc desc{};
				desc.Size = sizeof(SceneObjectGPUData) * MAX_DRAWS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage;
				desc.MemUsage = EBufferMemUsage::ECPUToGPU;

				m_FrameData[i].SceneObjectsBuffer = new RHIBuffer(desc);
				m_FrameData[i].SceneObjectsBuffer->InitRHI();
			}
			{
				BufferDesc desc{};
				desc.Size = sizeof(uint32_t) * MAX_SHADERS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage;
				desc.MemUsage = EBufferMemUsage::ECPUToGPU;

				m_FrameData[i].BatchOffsetsBuffer = new RHIBuffer(desc);
				m_FrameData[i].BatchOffsetsBuffer->InitRHI();
			}
			{
				BufferDesc desc{};
				desc.Size = sizeof(uint32_t) * MAX_DRAWS_PER_FRAME;
				desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst;
				desc.MemUsage = EBufferMemUsage::EGPUOnly;

				m_FrameData[i].VisibilityBuffer = new RHIBuffer(desc);
				m_FrameData[i].VisibilityBuffer->InitRHI();
			}

			m_FrameData[i].LightsOffset = 0;
			m_FrameData[i].ObjectsOffset = 0;
			m_FrameData[i].DrawCountOffset = 0;

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
			shader->SetTextureUAV(BRDFResources.OutTexture, m_BRDFLut->GetTextureView());

			GRHIDevice->ImmediateSubmit([&, this](RHICommandBuffer* cmd) {

				GRHIDevice->BarrierTexture2D(cmd, m_BRDFLut, EGPUAccessFlags::ENone, EGPUAccessFlags::EUAVCompute);
				GRHIDevice->BindShader(cmd, shader);

				uint32_t groupCount = GetComputeGroupCount(BRDF_LUT_SIZE, 8);
				GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

				GRHIDevice->BarrierTexture2D(cmd, m_BRDFLut, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ESRV);
				});
		}

		m_FrameCount = 0;
	}

	FrameRenderer::~FrameRenderer() {

		for (int i = 0; i < 2; i++) {

			m_ExecQueue[i].Flush();

			m_FrameData[i].LightsBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].LightsBuffer;
			m_FrameData[i].DrawCommandsBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].DrawCommandsBuffer;
			m_FrameData[i].DrawCountsBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].DrawCountsBuffer;
			m_FrameData[i].SceneObjectsBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].SceneObjectsBuffer;
			m_FrameData[i].BatchOffsetsBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].BatchOffsetsBuffer;
			m_FrameData[i].VisibilityBuffer->ReleaseRHIImmediate();
			delete m_FrameData[i].VisibilityBuffer;

			m_CommandBuffers[i]->ReleaseRHIImmediate();
			delete m_CommandBuffers[i];
		}

		m_BRDFLut->ReleaseRHIImmediate();
		delete m_BRDFLut;
	}

	void FrameRenderer::RenderScene(const SceneRenderProxy& scene, RenderContext& context, std::vector<SceneRenderFeature*>& features) {

		RDGBuilder builder = RDGBuilder();
		uint32_t frameIndex = m_FrameCount % 2;

		RHIBindingSet* sceneDataSet = new RHIBindingSet(GShaderManager->GetSceneLayout());
		sceneDataSet->InitRHI();

		for (auto& feature : features) {

			feature->BuildGraph(&builder, &scene, context, sceneDataSet);
		}

		builder.Execute(m_CommandBuffers[frameIndex]);

		PushToExecQueue([sceneDataSet]() {

			sceneDataSet->ReleaseRHI();
			delete sceneDataSet;
			});
	}

	void FrameRenderer::RenderSwapchain(uint32_t width, uint32_t height, RHITexture2D* fillTexture) {

		if (GApplication->IsUsingImGui()) {
			ImGui::End();
			ImGui::Render();
		}

		GRHIDevice->DrawSwapchain(m_CommandBuffers[m_FrameCount % 2], width, height, GApplication->IsUsingImGui(), fillTexture);
		m_FrameCount++;
	}

	void FrameRenderer::BeginFrame() {

		uint32_t frameIndex = m_FrameCount % 2;
		RHICommandBuffer* cmd = m_CommandBuffers[frameIndex];

		Timer gpuTimer;
		GRHIDevice->WaitForFrameCommandBuffer(cmd);
		ENGINE_INFO("Waited for gpu: {0} ms", gpuTimer.GetElapsedMs());

		m_ExecQueue[frameIndex].Flush();

		FrameData& frameData = m_FrameData[frameIndex];
		frameData.ObjectsOffset = 0;
		frameData.LightsOffset = 0;
		frameData.DrawCountOffset = 0;

		GRHIDevice->BeginFrameCommandBuffer(cmd);
		GRHIDevice->FillBuffer(cmd, frameData.DrawCountsBuffer, frameData.DrawCountsBuffer->GetSize(), 0, 0, EGPUAccessFlags::EIndirectArgs, EGPUAccessFlags::EUAVCompute);

		FrameData& prevFrameData = GetPrevFrameData();
		GRHIDevice->BarrierBuffer(cmd, prevFrameData.VisibilityBuffer, prevFrameData.VisibilityBuffer->GetSize(), 0, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ESRVCompute);

		// imgui
		if (GApplication->IsUsingImGui()) {

			GRHIDevice->BeginGuiNewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			static bool dockOpen = true;
			static bool opt_fullscreen = true;
			static bool opt_padding = false;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			if (opt_fullscreen)
			{
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}
			else
			{
				dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
			}

			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			if (!opt_padding)
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace", &dockOpen, window_flags);
			if (!opt_padding)
				ImGui::PopStyleVar();

			if (opt_fullscreen)
				ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}
		}
	}
}