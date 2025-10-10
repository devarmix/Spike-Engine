#include <imgui/imgui.h>
#include <Engine/Core/Core.h>
#include <Engine/World/World.h>
#include <Editor/Renderer/EditorCamera.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/World/Entity.h>

namespace Spike {

	namespace EditorUI {

		void AddFont(const std::string& name, ImFont* font);

		struct FontDesc {
			std::filesystem::path Path;
			float Size;
		};

		void AddFont(const std::string& name, const FontDesc& desc);
		
		struct ScopedFont {
			ScopedFont(const std::string& name);
			~ScopedFont();

		private:
			bool m_Valid;
		};
	}

	class WorldViewportWidget {
	public:
		WorldViewportWidget();
		~WorldViewportWidget();

		void Tick(float deltaTime);

	private:
		Ref<World> m_World;
		RHITexture2D* m_Output;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		Ref<CubeTexture> m_EnvMap;
		Ref<CubeTexture> m_IrrMap;
		Ref<CubeTexture> m_Skybox;

		EditorCamera m_Camera;
	};
}