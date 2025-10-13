#include <Editor/Renderer/Widget.h>
#include <Engine/Core/Log.h>
#include <Editor/Project/Project.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/DefaultFeatures.h>
#include <Engine/World/Components.h>
#include <Generated/DeferredPBR.h>

#include <random>
#include <imgui/imgui.h>

inline static std::map<std::string, ImFont*> s_Fonts;

void Spike::EditorUI::AddFont(const std::string& name, ImFont* font) {
	s_Fonts[name] = font;
}

void Spike::EditorUI::AddFont(const std::string& name, const FontDesc& desc) {

	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = io.Fonts->AddFontFromFileTTF(desc.Path.string().c_str(), desc.Size);

	s_Fonts[name] = font;
}

namespace Spike {

	WorldViewportWidget::WorldViewportWidget() : m_Output(nullptr) {

		m_Skybox = GRegistry->LoadAsset(UUID(13304147204958863046));
	    m_EnvMap = GRegistry->LoadAsset(UUID(7252836427571175987));
		m_IrrMap = GRegistry->LoadAsset(UUID(17015075508748921218));

		m_World = World::Create();
		// cube -  10020809986100263309

		Ref<Mesh> meshes[1] = {
			GRegistry->LoadAsset(UUID(10020809986100263309)).As<Mesh>(),
			//GRegistry->LoadAsset(UUID(17952529003848048510)).As<Mesh>(),
			//GRegistry->LoadAsset(UUID(11465955512769156728)).As<Mesh>(),
			//GRegistry->LoadAsset(UUID(2728495303558564767)).As<Mesh>()
		};

		ShaderDesc desc{};
		desc.Type = EShaderType::EGraphics;
		desc.Name = "DeferredPBR";
		desc.ColorTargetFormats = { ETextureFormat::ERGBA16F, ETextureFormat::ERGBA16F, ETextureFormat::ERGBA8U };
		desc.EnableDepthTest = true;
		desc.EnableDepthWrite = true;
		desc.CullBackFaces = true;
		desc.FrontFace = EFrontFace::ECounterClockWise;

		Ref<Material> mat = Material::Create(EMaterialSurfaceType::EOpaque, desc);
		{
			Ref<Texture2D> tex = GRegistry->LoadAsset(UUID(17935795566627736296)).As<Texture2D>();
			mat->SetTextureSRV(DeferredPBRMaterialResources.AlbedoMap, tex);
		} 

		for (int x = 0; x < 5; x++) {
			for (int y = 0; y < 5; y++) {
				for (int z = 0; z < 5; z++) {

					auto entity = m_World->CreateEntity();
					auto& mc = entity.AddComponent<StaticMeshComponent>();
					mc.PushMaterial(mat);
					mc.SetMesh(meshes[0]);

					auto& tc = entity.GetComponent<TransformComponent>();
					//tc.SetScale({ 0.05f, 0.05f, 0.05f });
					tc.SetPosition({ x * 4 + rand() % 5, y * 4 + rand() % 5, z * 4 + rand() % 5 });
					tc.SetRotation({ rand() % 360, rand() % 360, rand() % 360 });
				}
			}
		}
	}

	WorldViewportWidget::~WorldViewportWidget() {
		SafeRHIResourceRelease(m_Output);
	}

	void WorldViewportWidget::Tick(float deltaTime) {
		ImGui::Begin("World");
		ImVec2 size = ImGui::GetContentRegionAvail();

		if (size.x >= 1 && size.y >= 1) {

			if (!m_Output || m_Width != size.x || m_Height != size.y) {

				if (m_Output) {
					SafeRHIResourceRelease(m_Output);
				}

				SamplerDesc samplDesc{};
				samplDesc.Filter = ESamplerFilter::EBilinear;
				samplDesc.AddressU = ESamplerAddress::EClamp;
				samplDesc.AddressV = ESamplerAddress::EClamp;
				samplDesc.AddressW = ESamplerAddress::EClamp;

				Texture2DDesc desc{};
				desc.Width = m_Width = size.x;
				desc.Height = m_Height = size.y;
				desc.Format = ETextureFormat::ERGBA16F;
				desc.NumMips = 1;
				desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::EColorTarget | ETextureUsageFlags::ECopyDst | ETextureUsageFlags::EStorage;
				desc.SamplerDesc = samplDesc;

				m_Output = new RHITexture2D(desc);
				SafeRHIResourceInit(m_Output);
			}

			m_Camera.Tick(deltaTime);
			m_Camera.SetViewportHovered(ImGui::IsWindowHovered());

			CameraDrawData camData{};
			camData.View = m_Camera.GetViewMatrix();
			camData.Proj = m_Camera.GetProjectionMatrix(size.x / size.y);
			camData.Position = m_Camera.GetPosition();
			camData.NearProj = m_Camera.GetCameraNearProj();
			camData.FarProj = m_Camera.GetCameraFarProj();

			RenderContext context{};
			context.OutTexture = m_Output;
			context.EnvironmentTexture = m_EnvMap->GetResource();
			context.IrradianceTexture = m_IrrMap->GetResource();

			RHIWorldProxy* proxy = m_World->GetProxy();
			SUBMIT_RENDER_COMMAND(([=]() {
				std::vector<EFeatureType> features{
					EFeatureType::EGBuffer,
					EFeatureType::EDeferredLightning,
					EFeatureType::ESkybox,
					//GFrameRenderer->LoadFeature(EFeatureType::ESSAO),
					EFeatureType::ESMAA,
					EFeatureType::EBloom,
					EFeatureType::EToneMap
				};

				GFrameRenderer->RenderWorld(proxy, context, camData, features);
				}));

			ImGui::Image((ImTextureID)m_Output, size);
		}

		ImGui::End();
	}


	namespace EditorUI {

		ScopedFont::ScopedFont(const std::string& name) {
			
			auto it = s_Fonts.find(name);
			if (it != s_Fonts.end()) {
				ImGui::PushFont(it->second);
				m_Valid = true;
			}
			else {
				ENGINE_ERROR("Font: {} wasnt registered!", name);
				m_Valid = false;
			}
		}

		ScopedFont::~ScopedFont() {
			if (m_Valid) {
				ImGui::PopFont();
			}
		}
	}
}