#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Core/Window.h>
#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/RenderResource.h>
#include <Engine/Multithreading/RenderThread.h>

#include <imgui.h>
using namespace SpikeEngine;

namespace Spike {

	// utility function
	uint32_t RoundUpToPowerOfTwo(float value);

	class GBufferResource : public RenderResource {
	public:

		GBufferResource(uint32_t width, uint32_t height) 
			: m_AlbedoMapData(nullptr), m_MaterialMapData(nullptr), m_DepthMapData(nullptr),
		      m_NormalMapData(nullptr), m_Width(width), m_Height(height) 
		{
			m_DepthPyramidSize = RoundUpToPowerOfTwo((float)glm::max(width, height));
		}

		virtual ~GBufferResource() override {}

		virtual void InitGPUData() override;
		virtual void ReleaseGPUData() override;

		ResourceGPUData* GetAlbedoMapData() { return m_AlbedoMapData; }
		ResourceGPUData* GetMaterialMapData() { return m_MaterialMapData; }
		ResourceGPUData* GetDepthMapData() { return m_DepthMapData; }
		ResourceGPUData* GetNormalMapData() { return m_NormalMapData; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		uint32_t GetPyramidSize() const { return m_DepthPyramidSize; }

	private:

		ResourceGPUData* m_AlbedoMapData;
		ResourceGPUData* m_MaterialMapData;
		ResourceGPUData* m_DepthMapData;
		ResourceGPUData* m_NormalMapData;

		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_DepthPyramidSize;
	};

	struct RenderContext {
		
		GBufferResource* GBuffer;
		Texture2DResource* OutTexture;

		CubeTextureResource* EnvironmentTexture;
		CubeTextureResource* IrradianceTexture;
	};

	struct alignas(16) SceneDrawData {

		glm::mat4 View;
		glm::mat4 Proj;
		glm::mat4 ViewProj;
		glm::mat4 InverseProj;
		glm::mat4 InverseView;

		glm::vec4 CameraPos;

		float AmbientIntensity;
		float NearProj;
		float FarProj;

		float pad0;
	};
	
	struct SceneObjectProxy {

		MeshResource* Mesh;
		MaterialResource* Material;

		uint32_t FirstIndex;
		uint32_t IndexCount;

		int LastVisibilityIndex;
		int CurrentVisibilityIndex;

		glm::vec3 BoundsOrigin;
		glm::vec3 BoundsExtents;
		float BoundsRadius;

		glm::mat4 GlobalTransform;
	};

	struct alignas(16) SceneLightProxy {

		glm::vec4 Position;
		glm::vec4 Direction; // for directional and spot
		glm::vec4 Color;

		float Intensity;
		int Type; // 0 = directional, 1 = point, 2 = spot
		float InnerConeCos;
		float OuterConeCos;
	};

	struct SceneRenderProxy {

		std::vector<SceneObjectProxy> Objects;
		std::vector<SceneLightProxy> Lights;

		SceneDrawData DrawData;
	};

	class GfxDevice {
	public:
		virtual ~GfxDevice() = default;

		static void Create(Window* window, bool useImGui = false);

		virtual ResourceGPUData* CreateTexture2DGPUData(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData) = 0;
		virtual void DestroyTexture2DGPUData(ResourceGPUData* data) = 0;

		virtual ResourceGPUData* CreateCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture) = 0;
		virtual ResourceGPUData* CreateFilteredCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode) = 0;
		virtual void DestroyCubeTextureGPUData(ResourceGPUData* data) = 0;
		
		virtual ResourceGPUData* CreateMaterialGPUData(EMaterialSurfaceType surfaceType) = 0;
		virtual void DestroyMaterialGPUData(ResourceGPUData* data) = 0;

		virtual ResourceGPUData* CreateMeshGPUData(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices) = 0;
		virtual void DestroyMeshGPUData(ResourceGPUData* data) = 0;

		virtual ResourceGPUData* CreateDepthMapGPUData(uint32_t width, uint32_t height, uint32_t depthPyramidSize) = 0;
		virtual void DestroyDepthMapGPUData(ResourceGPUData* data) = 0;

		virtual void SetMaterialScalarParameter(MaterialResource* material, uint8_t dataBindIndex, float newValue) = 0;
		virtual void SetMaterialColorParameter(MaterialResource* material, uint8_t dataBindIndex, Vector4 newValue) = 0;
		virtual void SetMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex, Texture2DResource* newValue) = 0;
		virtual void AddMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) = 0;
		virtual void RemoveMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) = 0;

		virtual ImTextureID CreateStaticGuiTexture(Texture2DResource* texture) = 0;
		virtual void DestroyStaticGuiTexture(ImTextureID id) = 0;
		virtual ImTextureID CreateDynamicGuiTexture(Texture2DResource* texture) = 0;

		virtual void DrawSceneIndirect(SceneRenderProxy* scene, RenderContext renderContext) = 0;
		virtual void DrawSwapchain(uint32_t width, uint32_t height, bool drawGui = false, Texture2DResource* fillTexture = nullptr) = 0;
		virtual void BeginFrameCommandRecording() = 0;

	public:

		Ref<Texture2D> DefErrorTexture2D;
	};

	// global graphics device pointer
	extern GfxDevice* GGfxDevice;
}
