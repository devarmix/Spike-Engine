#include <Editor/Renderer/Widget.h>
#include <Engine/Core/Log.h>
#include <Editor/Project/Project.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/DefaultFeatures.h>
#include <Engine/World/Components.h>
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

		Ref<Mesh> mesh = GRegistry->LoadAsset(UUID(10020809986100263309)).As<Mesh>();

		m_Entity = new Entity(m_World);
		auto& c = m_Entity->AddComponent<StaticMeshComponent>();

		{
			ShaderDesc desc{};
			desc.Type = EShaderType::EGraphics;
			desc.Name = "DeferredPBR";
			desc.ColorTargetFormats = {ETextureFormat::ERGBA16F, ETextureFormat::ERGBA16F, ETextureFormat::ERGBA8U};
			desc.EnableDepthTest = true;
			desc.EnableDepthWrite = true;
			//desc.CullBackFaces = true;
			desc.FrontFace = EFrontFace::EClockWise;

			Ref<Material> mat = Material::Create(EMaterialSurfaceType::EOpaque, desc);
			c.PushMaterial(mat);
		}

		c.SetMesh(mesh); //PROBLEM
	}

	WorldViewportWidget::~WorldViewportWidget() {
		SafeRHIResourceRelease(m_Output);
		delete m_Entity;
	}

	void WorldViewportWidget::Tick(float deltaTime) {
		static bool open = true;

		ImGui::Begin("World", &open, ImGuiWindowFlags_NoMove);
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
				std::vector<RenderFeature*> features{
					GFrameRenderer->LoadFeature(EFeatureType::EGBuffer),
					GFrameRenderer->LoadFeature(EFeatureType::EDeferredLightning),
					GFrameRenderer->LoadFeature(EFeatureType::ESkybox),
					GFrameRenderer->LoadFeature(EFeatureType::ESSAO),
					GFrameRenderer->LoadFeature(EFeatureType::ESMAA),
					GFrameRenderer->LoadFeature(EFeatureType::EBloom),
					GFrameRenderer->LoadFeature(EFeatureType::EToneMap)
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