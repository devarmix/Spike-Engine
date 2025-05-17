#include <Engine/Layers/RendererLayer.h>
#include <Engine/Events/ApplicationEvents.h>

#include <Platforms/Vulkan/VulkanRenderer.h>

namespace Spike {

	RendererLayer::~RendererLayer() {
		
		vkDeviceWaitIdle(VulkanRenderer::Device.Device);

		Meshes.clear();
		VulkanRenderer::Cleanup();
	}


	void RendererLayer::OnAttach() {

		Meshes = Mesh::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/models/monkey.glb");

		//Ref<Texture> albedoMap = Texture::Create("res/textures/sloppy-mortar-stone-wall_albedo.png");
		//Ref<Texture> normalMap = Texture::Create("res/textures/sloppy-mortar-stone-wall_normal-ogl.png");
		//Ref<Texture> aoMap = Texture::Create("res/textures/sloppy-mortar-stone-wall_ao.png");
		//Ref<Texture> metMap = Texture::Create("res/textures/sloppy-mortar-stone-wall_metallic.png");
		//Ref<Texture> rougMap = Texture::Create("res/textures/sloppy-mortar-stone-wall_roughness.png");

		Ref<Material> PBRMat = Material::Create();

		//PBRMat->AddTextureParameter("AlbedoMap", 0, albedoMap);
		//PBRMat->AddTextureParameter("NormalMap", 1, normalMap);
		//PBRMat->AddTextureParameter("AOMap", 2, aoMap);
		//PBRMat->AddTextureParameter("MetallicMap", 3, metMap);
		//PBRMat->AddTextureParameter("RoughnessMap", 4, rougMap);

		for (int i = 0; i < Meshes[0]->SubMeshes.size(); i++) {

			Meshes[0]->SubMeshes[i].Material = PBRMat;
		}
	}

	void RendererLayer::OnDetach() {}

	void RendererLayer::OnUpdate(float deltaTime) {

		VulkanRenderer::SubmitMesh(Meshes[0], glm::mat4{ 1.f });
		//VulkanRenderer::SubmitLight(10, 10, glm::vec4{ 0.3f, 0.5f, 0.7f, 1.0f }, glm::vec3{ 3, 3, 3 });
		VulkanRenderer::Draw();
	}

	void RendererLayer::OnEvent(Event& event) {

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RendererLayer::OnWindowResize));
		dispatcher.Dispatch<SDLEvent>(BIND_EVENT_FN(RendererLayer::OnSDLEvent));

		dispatcher.Dispatch<SceneViewportResizeEvent>(BIND_EVENT_FN(RendererLayer::OnSceneViewportResize));
		dispatcher.Dispatch<SceneViewportMinimizeEvent>(BIND_EVENT_FN(RendererLayer::OnSceneViewportMinimize));
		dispatcher.Dispatch<SceneViewportRestoreEvent>(BIND_EVENT_FN(RendererLayer::OnSceneViewportRestore));
	}

	bool RendererLayer::OnWindowResize(const WindowResizeEvent& event) {

		VulkanRenderer::ResizeSwapchain(event.GetWidth(), event.GetHeight());
		return false;
	}

	bool RendererLayer::OnSDLEvent(SDLEvent& event) {

		VulkanRenderer::PoolImGuiEvents(event.GetEvent());

		return false;
	}

	bool RendererLayer::OnSceneViewportResize(const SceneViewportResizeEvent& event) {

		if (!VulkanRenderer::ViewportMinimized)
			VulkanRenderer::ResizeViewport(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool RendererLayer::OnSceneViewportMinimize(const SceneViewportMinimizeEvent& event) {

		VulkanRenderer::ViewportMinimized = true;

		return false;
	}

	bool RendererLayer::OnSceneViewportRestore(const SceneViewportRestoreEvent& event) {

		VulkanRenderer::ViewportMinimized = false;

		return false;
	}
}