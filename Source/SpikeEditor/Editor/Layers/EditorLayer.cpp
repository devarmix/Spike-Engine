#include <Editor/Layers/EditorLayer.h>
#include <Engine/Core/Application.h>

namespace SpikeEditor {

	EditorLayer::~EditorLayer() {

		SafeRenderResourceRelease(m_GBuffer);
		m_LoadedMeshes.clear();
	}

	void EditorLayer::OnUpdate(float deltaTime) {

		if (!(m_Width == m_SceneViewport->GetWidth() && m_Height == m_SceneViewport->GetHeight()) && m_Width > 0 && m_Height > 0) {

			// resize
			SafeRenderResourceRelease(m_GBuffer);

			m_GBuffer = new GBufferResource(m_Width, m_Height);
			SafeRenderResourceInit(m_GBuffer);

			m_SceneViewport->ReleaseResource();
		    m_SceneViewport->CreateResource(m_Width, m_Height, EFormatRGBA16SFloat, EUsageFlagColorAttachment | EUsageFlagTransferSrc | EUsageFlagSampled);
		}

		Texture2DResource* outTexResource = m_SceneViewport->GetResource();
		GBufferResource* gbufferResource = m_GBuffer;
		CubeTextureResource* radianceMap = m_EnvironmentTexture->GetResource();
		CubeTextureResource* irradianceMap = m_IrradianceTexture->GetResource();
		MeshResource* meshResource = m_LoadedMeshes[0]->GetResource();
		MaterialResource* materialResource = m_LoadedMeshes[0]->SubMeshes[0].Material->GetResource();

		uint32_t FirstIndex = m_LoadedMeshes[0]->SubMeshes[0].FirstIndex;
		uint32_t IndexCount = m_LoadedMeshes[0]->SubMeshes[0].IndexCount;

		Vector3 BoundsOrigin = m_LoadedMeshes[0]->SubMeshes[0].BoundsOrigin;
		Vector3 BoundsExtents = m_LoadedMeshes[0]->SubMeshes[0].BoundsExtents;
		float BoundsRadius = m_LoadedMeshes[0]->SubMeshes[0].BoundsRadius;

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

			SceneRenderProxy* renderProxy = new SceneRenderProxy();

			{
				SceneLightProxy light{};

				light.Intensity = 10;
				light.Color = { 0.9f, 0.2f, 1.0f, 1.0f };
				light.Position = { -3.f, 0.f, -5.f, 0.f };
				light.Type = 1;

				renderProxy->Lights.push_back(light);
			}

			{
				SceneLightProxy light{};

				light.Intensity = 12;
				light.Color = { 0.3f, 0.9f, 1.0f, 1.0f };
				light.Position = { 5.f, 4.f, 4.f, 0.0f };
				light.Type = 1;

				renderProxy->Lights.push_back(light);
			}

			Timer cpuTimer;

			uint32_t sideSize = 25;
			uint32_t objectStep = 3;
			renderProxy->Objects.reserve(sideSize * sideSize * sideSize);

			int visibilityIndex = 0;

			for (uint32_t a = 0; a < sideSize; a++) {

				for (uint32_t b = 0; b < sideSize; b++) {

					for (uint32_t c = 0; c < sideSize; c++) {

						{
							SceneObjectProxy object{};

							object.Mesh = meshResource;
							object.Material = materialResource;
							object.FirstIndex = FirstIndex;
							object.IndexCount = IndexCount;
							object.LastVisibilityIndex = visibilityIndex;
							object.CurrentVisibilityIndex = visibilityIndex;
							object.GlobalTransform = glm::translate(glm::mat4(1.f), glm::vec3(a * objectStep, b * objectStep, c * objectStep));
							object.BoundsOrigin = { BoundsOrigin.x, BoundsOrigin.y, BoundsOrigin.z};
							object.BoundsExtents = { BoundsExtents.x, BoundsExtents.y, BoundsExtents.z };
							object.BoundsRadius = BoundsRadius;

							renderProxy->Objects.push_back(object);
						}

						visibilityIndex++;
					}
				}
			}

			ENGINE_INFO("Waited for scene: {} ms.", cpuTimer.GetElapsedMs());

			{
				float aspect = (float)m_Width / (float)m_Height;

				renderProxy->DrawData.CameraPos = { m_EditorCamera.GetPosition(), 0.0f};
				renderProxy->DrawData.View = m_EditorCamera.GetViewMatrix();
				renderProxy->DrawData.Proj = m_EditorCamera.GetProjectionMatrix(aspect);
				//renderProxy->DrawData.Proj[1][1] *= -1.0f;
				renderProxy->DrawData.ViewProj = renderProxy->DrawData.Proj * renderProxy->DrawData.View;
				renderProxy->DrawData.InverseProj = glm::inverse(renderProxy->DrawData.Proj);
				renderProxy->DrawData.InverseView = glm::inverse(renderProxy->DrawData.View);
				renderProxy->DrawData.NearProj = m_EditorCamera.GetCameraNearProj();
				renderProxy->DrawData.FarProj = m_EditorCamera.GetCameraFarProj();
			}

			RenderContext context{};
			context.EnvironmentTexture = radianceMap;
			context.GBuffer = gbufferResource;
			context.IrradianceTexture = irradianceMap;
			context.OutTexture = outTexResource;

			GGfxDevice->DrawSceneIndirect(renderProxy, context);

			ImTextureID viewID = GGfxDevice->CreateDynamicGuiTexture(outTexResource);
			ImGui::Image(viewID, { (float)outTexResource->GetWidth(), (float)outTexResource->GetHeight() });

			ImGui::End();
			}));
	}

	void EditorLayer::OnAttach() {

		m_SceneViewport = Texture2D::Create(1920, 1080, EFormatRGBA16SFloat, EUsageFlagColorAttachment | EUsageFlagTransferSrc | EUsageFlagSampled);

		m_SkyboxTexture = CubeTexture::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/AmbienceExposure4k.hdr", 1024);
		m_IrradianceTexture = CubeTexture::CreateFiltered(64, EFormatRGBA32SFloat, EUsageFlagTransferDst | EUsageFlagSampled, m_SkyboxTexture, EFilterIrradiance);
		m_EnvironmentTexture = CubeTexture::CreateFiltered(m_SkyboxTexture->GetSize(), EFormatRGBA32SFloat, EUsageFlagTransferDst | EUsageFlagSampled, m_SkyboxTexture, EFilterRadiance);

		m_LoadedMeshes = Mesh::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/models/plane2.glb");
		{
			float roughness = 0.0f;
			float metallic = 1.0f;

			Ref<Material> PBRMat = Material::Create(ESurfaceTypeOpaque);

			//PBRMat->AddScalarParameter("Roughness", 0, roughness);
			//PBRMat->AddScalarParameter("Metallic", 1, metallic);

			Ref<Texture2D> albedoMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_albedo.png");
			Ref<Texture2D> normalMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_normal-ogl.png");
			Ref<Texture2D> aoMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_ao.png");
			Ref<Texture2D> metMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_metallic.png");
			Ref<Texture2D> rougMap = Texture2D::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/metal-siding-base_roughness.png");

			PBRMat->AddTextureParameter("AlbedoMap", 0, albedoMap);
			PBRMat->AddTextureParameter("NormalMap", 1, normalMap);
			PBRMat->AddTextureParameter("AOMap", 2, aoMap);
			PBRMat->AddTextureParameter("MetallicMap", 3, metMap);
			PBRMat->AddTextureParameter("RoughnessMap", 4, rougMap);

			for (int i = 0; i < m_LoadedMeshes[0]->SubMeshes.size(); i++) {

				m_LoadedMeshes[0]->SubMeshes[i].Material = PBRMat;
			}
		}

		m_GBuffer = new GBufferResource(1920, 1080);
		SafeRenderResourceInit(m_GBuffer);
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