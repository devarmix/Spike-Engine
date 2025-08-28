#include <Editor/Layers/EditorLayer.h>
#include <Engine/Core/Application.h>
#include <Generated/DeferredPBR.h>
#include <Engine/Renderer/DefaultFeatures.h>

#include <Engine/Core/Stats.h>

namespace SpikeEditor {

	EditorLayer::~EditorLayer() {

		m_LoadedMeshes.clear();

		for (auto f : m_Features)
			delete f;

		m_Features.clear();
	}

	void EditorLayer::OnUpdate(float deltaTime) {

		if (!((float)m_Width == m_SceneViewport->GetSizeXYZ().x && (float)m_Height == m_SceneViewport->GetSizeXYZ().y) && m_Width > 0 && m_Height > 0) {

			SamplerDesc samplerDesc{};
			samplerDesc.Filter = ESamplerFilter::EBilinear;
			samplerDesc.AddressU = ESamplerAddress::EClamp;
			samplerDesc.AddressV = ESamplerAddress::EClamp;
			samplerDesc.AddressW = ESamplerAddress::EClamp;

			Texture2DDesc desc{};
			desc.Width = m_Width;
			desc.Height = m_Height;
			desc.Format = ETextureFormat::ERGBA16F;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopySrc | ETextureUsageFlags::ECopyDst | ETextureUsageFlags::EColorTarget;
			desc.SamplerDesc = samplerDesc;

			m_SceneViewport->ReleaseResource();
		    m_SceneViewport->CreateResource(desc);
		}

		RHITexture2D* outTexResource = m_SceneViewport->GetResource();
		RHICubeTexture* radianceMap = m_EnvironmentTexture->GetResource();
		RHICubeTexture* irradianceMap = m_IrradianceTexture->GetResource();

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			ImGui::Begin("Scene Viewport");

			m_ViewportHovered = ImGui::IsWindowHovered();

			m_EditorCamera.SetViewportHovered(m_ViewportHovered);

			// update camera
			m_EditorCamera.OnUpdate(deltaTime);

			// poll events
			ImVec2 size = ImGui::GetContentRegionAvail();

			if (m_Width != size.x || m_Height != size.y) {

				if ((size.x <= 0 || size.y <= 0) && !m_ViewportMinimized) {

					m_ViewportMinimized = true;
				}

				if (m_ViewportMinimized && (size.x > 0 && size.y > 0)) {

					m_ViewportMinimized = false;
				}

				if (size.x < 0) m_Width = 0;
				else m_Width = (uint32_t)size.x;

				if (size.y < 0) m_Height = 0;
				else m_Height = (uint32_t)size.y;
			}

			{
				float aspect = (float)m_Width / (float)m_Height;

				m_Scene.CameraData.Position = Vec4(m_EditorCamera.GetPosition(), 0.0f);
				m_Scene.CameraData.View = m_EditorCamera.GetViewMatrix();
				m_Scene.CameraData.Proj = m_EditorCamera.GetProjectionMatrix(aspect);
				m_Scene.CameraData.FarProj = m_EditorCamera.GetCameraFarProj();
				m_Scene.CameraData.NearProj = m_EditorCamera.GetCameraNearProj();
			}

			RenderContext context{};
			context.EnvironmentTexture = radianceMap;
			context.IrradianceTexture = irradianceMap;
			context.OutTexture = outTexResource;

			GFrameRenderer->RenderScene(m_Scene, context, m_Features);

			ImTextureID viewID = GRHIDevice->CreateDynamicGuiTexture(outTexResource->GetTextureView());
			ImGui::Image(viewID, { (float)outTexResource->GetSizeXYZ().x, (float)outTexResource->GetSizeXYZ().y });

			ImGui::End();

			ImGui::Begin("Stats");
			ImGui::Text("FPS: %.1f", Stats::Data.Fps);
			ImGui::Text("Frame Time: %.1f", Stats::Data.Frametime);
			ImGui::End();
			}));
	}

	void EditorLayer::OnAttach() {

		SamplerDesc samplerDesc{};
		samplerDesc.Filter = ESamplerFilter::EBilinear;
		samplerDesc.AddressU = ESamplerAddress::EClamp;
		samplerDesc.AddressV = ESamplerAddress::EClamp;
		samplerDesc.AddressW = ESamplerAddress::EClamp;

		{
			Texture2DDesc desc{};
			desc.Width = 1920;
			desc.Height = 1080;
			desc.Format = ETextureFormat::ERGBA16F;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopySrc | ETextureUsageFlags::ECopyDst | ETextureUsageFlags::EColorTarget;
			desc.SamplerDesc = samplerDesc;

			m_SceneViewport = Texture2D::Create(desc);
		}
		{
			m_SkyboxTexture = CubeTexture::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/AmbienceExposure4k.hdr", 1024, samplerDesc);

			CubeTextureDesc desc{};
			desc.Size = 64;
			desc.Format = ETextureFormat::ERGBA32F;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
			desc.NumMips = 1;
			desc.SamplerDesc = samplerDesc;
			desc.FilterMode = ECubeTextureFilterMode::EIrradiance;
			desc.SamplerTexture = m_SkyboxTexture->GetResource();

			m_IrradianceTexture = CubeTexture::Create(desc);

			desc.Size = 1024;
			desc.NumMips = GetNumTextureMips(1024, 1024);
			desc.FilterMode = ECubeTextureFilterMode::ERadiance;

			m_EnvironmentTexture = CubeTexture::Create(desc);
		}

		m_LoadedMeshes = Mesh::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/models/cube.glb");

		SamplerDesc mapsSamplerDesc{};
		mapsSamplerDesc.Filter = ESamplerFilter::ETrilinear;
		mapsSamplerDesc.AddressU = ESamplerAddress::ERepeat;
		mapsSamplerDesc.AddressV = ESamplerAddress::ERepeat;
		mapsSamplerDesc.AddressW = ESamplerAddress::ERepeat;
		mapsSamplerDesc.EnableAnisotropy = true;
		mapsSamplerDesc.MaxAnisotropy = 8.f;

		Ref<Texture2D> albedoMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_albedo.png", mapsSamplerDesc);
		Ref<Texture2D> normalMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_normal-ogl.png", mapsSamplerDesc);
		Ref<Texture2D> aoMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_ao.png", mapsSamplerDesc);
		Ref<Texture2D> metMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_metallic.png", mapsSamplerDesc);
		Ref<Texture2D> rougMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_roughness.png", mapsSamplerDesc);

		{
			float roughness = 0.0f;
			float metallic = 1.0f;

			ShaderDesc desc{};
			desc.Type = EShaderType::EGraphics;
			desc.Name = "DeferredPBR";
			desc.ColorTargetFormats = { ETextureFormat::ERGBA16F, ETextureFormat::ERGBA16F, ETextureFormat::ERGBA8U };
			desc.EnableDepthTest = true;
			desc.EnableDepthWrite = true;
			desc.CullBackFaces = true;
			desc.FrontFace = EFrontFace::ECounterClockWise;

			Ref<Material> PBRMat = Material::Create(EMaterialSurfaceType::EOpaque, desc);

			//PBRMat->AddScalarParameter("Roughness", 0, roughness);
			//PBRMat->AddScalarParameter("Metallic", 1, metallic);

			samplerDesc.Filter = ESamplerFilter::ETrilinear;
			samplerDesc.AddressU = ESamplerAddress::ERepeat;
			samplerDesc.AddressV = ESamplerAddress::ERepeat;
			samplerDesc.AddressW = ESamplerAddress::ERepeat;

			PBRMat->SetTextureSRV(DeferredPBRMaterialResources.AlbedoMap, albedoMap);
			PBRMat->SetTextureSRV(DeferredPBRMaterialResources.NormalMap, normalMap);
			PBRMat->SetTextureSRV(DeferredPBRMaterialResources.AOMap, aoMap);
			PBRMat->SetTextureSRV(DeferredPBRMaterialResources.MettalicMap, metMap);
			PBRMat->SetTextureSRV(DeferredPBRMaterialResources.RoughnessMap, rougMap);

			for (int i = 0; i < m_LoadedMeshes[0]->SubMeshes.size(); i++) {

				m_LoadedMeshes[0]->SubMeshes[i].Material = PBRMat;
			}
		}

		// create a scene to render
		{
			const SubMesh& sMesh = m_LoadedMeshes[0]->SubMeshes[0];
			RHIMesh* rhiMesh = m_LoadedMeshes[0]->GetResource();
			RHIMaterial* rhiMat = sMesh.Material->GetResource();

			EXECUTE_ON_RENDER_THREAD(([=, this]() {

				{
					SceneLightGPUData light{};

					light.Intensity = 30;
					light.Color = { 0.9f, 0.2f, 1.0f, 1.0f };
					light.Position = { 3.f, 0.f, -5.f, 0.f };
					light.Type = 1;

					m_Scene.Lights.push_back(light);
				}
				{
					SceneLightGPUData light{};

					light.Intensity = 12;
					light.Color = { 0.5f, 0.9f, 1.0f, 1.0f };
					light.Position = { 5.f, 4.f, 4.f, 0.0f };
					light.Type = 1;

					m_Scene.Lights.push_back(light);
				}

				uint32_t sideSize = 25; 
				uint32_t objectStep = 3;
				m_Scene.Objects.reserve(sideSize * sideSize * sideSize);

				int visibilityIndex = 0;

				for (uint32_t a = 0; a < sideSize; a++) {

					for (uint32_t b = 0; b < sideSize; b++) {

						for (uint32_t c = 0; c < sideSize; c++) {

							{
								SceneObjectGPUData object{};
								object.BoundsOrigin = Vec4(sMesh.BoundsOrigin, sMesh.BoundsRadius);
								object.BoundsExtents = Vec4(sMesh.BoundsExtents, 1.f);
								object.GlobalTransform = glm::translate(Mat4x4(1.f), Vec3(a * objectStep, b * objectStep, c * objectStep));
								object.InverseTransform = glm::inverse(object.GlobalTransform);
								object.FirstIndex = sMesh.FirstIndex;
								object.IndexCount = sMesh.IndexCount;
								object.IndexBufferAddress = rhiMesh->GetIndexBuffer()->GetGPUAddress();
								object.VertexBufferAddress = rhiMesh->GetVertexBuffer()->GetGPUAddress();
								object.MaterialBufferIndex = rhiMat->GetDataIndex();
								object.DrawBatchID = 0;
								object.LastVisibilityIndex = visibilityIndex;
								object.CurrentVisibilityIndex = visibilityIndex;

								m_Scene.Objects.push_back(object); 
							}

							visibilityIndex++;
						}
					}
				}
				{
					DrawBatch batch{};
					batch.CommandsCount = m_Scene.Objects.size();
					batch.Shader = rhiMat->GetShader();

					m_Scene.Batches.push_back(batch);
				}
				{
					m_Features.push_back(new GBufferFeature());
				    m_Features.push_back(new DeferredLightingFeature());
					m_Features.push_back(new SkyboxFeature());
					m_Features.push_back(new SSAOFeature());
					m_Features.push_back(new BloomFeature());
					m_Features.push_back(new ToneMapFeature());
				}
				}));
		}
	}

	void EditorLayer::OnDetach() {}

	void EditorLayer::OnEvent(const GenericEvent& event) {

		m_EditorCamera.OnEvent(event);

		EventHandler handler(event);
		handler.Handle<MouseButtonPressEvent>(BIND_FUNCTION(EditorLayer::OnMouseButtonPress));
	}

	bool EditorLayer::OnMouseButtonPress(const MouseButtonPressEvent& event) {

		MouseButtonPressEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (e.GetButton() == SDL_BUTTON_RIGHT && m_ViewportHovered) {

				// if controlling camera, set this window as focused
				ImGui::SetWindowFocus(GetName().c_str());
			}
			}));

		return false;
	}
}