#include <Editor/Project/Project.h>
#include <Editor/Core/Editor.h>
#include <Engine/Core/Log.h>
#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Core/Application.h>

#include <Generated/CubeMapFiltering.h>
#include <Generated/CubeMapGen.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>

namespace Spike {

	Ref<Asset> EditorRegistry::LoadAsset(UUID id) {

		{
			auto it = m_LoadedAssets.find(id);
			if (it != m_LoadedAssets.end()) {
				return m_LoadedAssets[id];
			}
		}
		{
			// not found, try loading
			auto it = m_Registry.find(id);
			if (it != m_Registry.end()) {

				BinaryReadStream stream(GEditor->GetProjectPath()/it->second.Path);
				if (!stream.IsOpen()) {
					ENGINE_ERROR("Failed to open asset binary from: {}", it->second.Path.string());
					return nullptr;
				}

				switch (it->second.Type)
				{
				case EAssetType::ETexture2D:
					return Texture2D::Create(stream, id).As<Asset>();
				case EAssetType::ECubeTexture:
					return CubeTexture::Create(stream, id).As<Asset>();
				case EAssetType::EMaterial:
					//return LoadMaterial(it->second.Path, id);
					assert(false);
				case EAssetType::EMesh:
					return Mesh::Create(stream, id).As<Asset>();
				default:
					ENGINE_ERROR("Invalid asset type: {}", (uint64_t)id);
					return nullptr;
				}
			}
			else {
				ENGINE_ERROR("Invalid UUID to load: {}", (uint64_t)id);
				return nullptr;
			}
		}
	}

	void EditorRegistry::UnloadAsset(UUID id) {

		auto it = m_LoadedAssets.find(id);
		if (it != m_LoadedAssets.end()) {
			m_LoadedAssets.erase(id);
		}
	}

	void EditorRegistry::ImportAsset(const AssetInfo& info) {
		UUID id = UUID::Generate();
		m_Registry[id] = info;

		ENGINE_INFO("Imported asset with id: {}", (uint64_t)id);
	}

	void EditorRegistry::RemoveAsset(UUID id) {

		auto it = m_Registry.find(id);
		if (it != m_Registry.end()) {
			m_Registry.erase(id);
		}
	}

	void EditorRegistry::Save() {

		std::filesystem::create_directories(GEditor->GetProjectPath());
		BinaryWriteStream stream(GEditor->GetProjectPath()/"Registry.db");
		if (!stream.IsOpen()) {
			ENGINE_ERROR("Failed to create registry bin file!");
		}
		else {
			stream << m_Registry;
		}
	}

	void EditorRegistry::Deserialize() {

		BinaryReadStream stream(GEditor->GetProjectPath() / "Registry.db");
		if (!stream.IsOpen()) {
			ENGINE_WARN("Failed to open registry bin file! Ignore this if project was just created");
		}
		else {
			stream >> m_Registry;
		}
	}
}


// utility
static void ProcessAssimpNode(aiNode* node, const aiScene* scene, const std::filesystem::path& path) {
	MeshDesc desc{};

	for (uint32_t s = 0; s < node->mNumMeshes; s++) {

		aiMesh* sMesh = scene->mMeshes[node->mMeshes[s]];
		SubMesh subMesh{};
		{
			for (uint32_t i = 0; i < sMesh->mNumVertices; i++) {

				Vertex v{};
				v.Position = Vec4(sMesh->mVertices[i].x, sMesh->mVertices[i].y, sMesh->mVertices[i].z, 0.f);

				Vec3 tempT = Vec3(sMesh->mTangents[i].x, sMesh->mTangents[i].y, sMesh->mTangents[i].z);
				Vec3 tempN = Vec3(sMesh->mNormals[i].x, sMesh->mNormals[i].y, sMesh->mNormals[i].z);
				Vec3 tempB = Vec3(sMesh->mBitangents[i].x, sMesh->mBitangents[i].y, sMesh->mBitangents[i].z);
				Vec3 tempNT = glm::cross(tempN, tempT);

				float tHandedness = glm::dot(tempNT, tempB) < 0.0f ? -1.0f : 1.0f;
				v.Normal = MathUtils::PackSignedVec4ToHalf(Vec4(tempN, 0.f));
				v.Tangent = MathUtils::PackSignedVec4ToHalf(Vec4(tempT, tHandedness));

				if (sMesh->mTextureCoords[0]) {
					v.UV0.x = sMesh->mTextureCoords[0][i].x;
					v.UV0.y = sMesh->mTextureCoords[0][i].y;
				}

				if (sMesh->mTextureCoords[1]) {
					v.UV1.x = sMesh->mTextureCoords[1][i].x;
					v.UV1.y = sMesh->mTextureCoords[1][i].y;
				}

				if (sMesh->mColors[0]) {
					v.Color = MathUtils::PackUnsignedVec4ToUint(Vec4(sMesh->mColors[0][i].r, sMesh->mColors[0][i].g, sMesh->mColors[0][i].b, sMesh->mColors[0][i].a));
				}
				else {
					v.Color = MathUtils::PackUnsignedVec4ToUint(Vec4(0.f));
				}

				desc.Vertices.push_back(std::move(v));
			}

			subMesh.FirstIndex = (uint32_t)desc.Indices.size();
			for (uint32_t i = 0; i < sMesh->mNumFaces; i++) {

				aiFace face = sMesh->mFaces[i];
				for (uint32_t f = 0; f < face.mNumIndices; f++) {
					desc.Indices.push_back(face.mIndices[f]);
					subMesh.IndexCount++;
				}
			}
		}

		desc.SubMeshes.push_back(std::move(subMesh));
	}

	// import a mesh asset
	if (desc.Vertices.size() > 0 && desc.Indices.size() > 0 && desc.SubMeshes.size() > 0) {
		EditorRegistry::AssetInfo info{};
		info.Type = EAssetType::EMesh;

		std::string name = node->mName.C_Str();
		name += ".asset";

		info.Path = std::filesystem::path(path / name);

		std::filesystem::path fullPath = GEditor->GetProjectPath() / info.Path;
		std::filesystem::create_directories(fullPath.parent_path());
		BinaryWriteStream stream(fullPath);

		if (!stream.IsOpen()) {
			ENGINE_ERROR("Failed to create mesh asset stream! Path: {}", fullPath.string());
			return;
		}
		stream << MESH_MAGIC << desc.Vertices << desc.Indices << desc.SubMeshes;
		GApplication->DispatchEvent<AssetImportedEvent>(info);
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		ProcessAssimpNode(node->mChildren[i], scene, path);
	}
}

void Spike::AssetImporter::ImportMesh(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath) {

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(sourcePath.string(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		ENGINE_ERROR("Failed to import mesh from: {0}, with error: {1}", sourcePath.string(), importer.GetErrorString());
		return;
	}
	ProcessAssimpNode(scene->mRootNode, scene, assetPath);
}

// utility
static void CompressTexture(RHIBuffer* out, RHITexture* tex, const std::vector<size_t>& mipSizes, const std::vector<Vec2Uint>& mipExtents, uint32_t numArrayLayers, ETextureFormat compFormat) {

	BufferDesc stagingBuffDesc{};
	stagingBuffDesc.Size = mipSizes[0] * numArrayLayers;
	stagingBuffDesc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopySrc;
	stagingBuffDesc.MemUsage = EBufferMemUsage::EGPUOnly;

	RHIBuffer* stagingBuffer = new RHIBuffer(stagingBuffDesc);
	stagingBuffer->InitRHI();

	ShaderDesc shaderDesc{};
	shaderDesc.Type = EShaderType::ECompute;

	switch (compFormat)
	{
	case ETextureFormat::ERGBBC1:
		shaderDesc.Name = "BC1Compress";
		break;
	case ETextureFormat::ERGBABC3:
		shaderDesc.Name = "BC3Compress";
		break;
	case ETextureFormat::ERGBC5:
		shaderDesc.Name = "BC5Compress";
		break;
	case ETextureFormat::ERGBABC6:
		shaderDesc.Name = "BC6Compress";
		break;
	default:
		break;
	}

	RHIShader* shader = GShaderManager->GetShaderFromCache(shaderDesc);
	std::vector<RHIBindingSet*> sets{};
	std::vector<RHITextureView*> views{};

	GRHIDevice->ImmediateSubmit([&](RHICommandBuffer* cmd) {

		size_t outOffset = 0;
		for (uint32_t m = 0; m < tex->GetNumMips(); m++) {

			TextureViewDesc viewDesc{};
			viewDesc.BaseArrayLayer = 0;
			viewDesc.BaseMip = m;
			viewDesc.NumArrayLayers = numArrayLayers;
			viewDesc.NumMips = 1;
			viewDesc.SourceTexture = tex;

			size_t dataSize = mipSizes[m] * numArrayLayers;

			RHITextureView* view = views.emplace_back(new RHITextureView(viewDesc));
			view->InitRHI();

			RHIBindingSet* set = sets.emplace_back(new RHIBindingSet(shader->GetLayouts()[0]));
			set->InitRHI();
			{
				set->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, view, EGPUAccessFlags::ESRV);
				set->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, tex->GetSampler());
				set->AddBufferWrite(2, 0, EShaderResourceType::EBufferUAV, stagingBuffer, dataSize, 0);
			}

			uint32_t groupX = GetComputeGroupCount(mipExtents[m].x, 8);
			uint32_t groupY = GetComputeGroupCount(mipExtents[m].y, 8);

			GRHIDevice->BindShader(cmd, shader, { set });
			GRHIDevice->DispatchCompute(cmd, groupX, groupY, numArrayLayers);
			{
				GRHIDevice->BarrierBuffer(cmd, stagingBuffer, dataSize, 0, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ECopySrc);
				GRHIDevice->CopyBuffer(cmd, stagingBuffer, out, 0, outOffset, dataSize);
				GRHIDevice->BarrierBuffer(cmd, stagingBuffer, stagingBuffer->GetSize(), 0, EGPUAccessFlags::ECopySrc, EGPUAccessFlags::EUAVCompute);
			}
			outOffset += dataSize;
		}
		});

	stagingBuffer->ReleaseRHIImmediate();
	delete stagingBuffer;

	for (uint32_t m = 0; m < tex->GetNumMips(); m++) {
		sets[m]->ReleaseRHIImmediate();
		views[m]->ReleaseRHIImmediate();

		delete sets[m];
		delete views[m];
	}
}

static void CalculateTextureMipData(std::vector<size_t>& mipSizes, std::vector<Vec2Uint>& mipExtents, size_t& byteSize, uint32_t numMips, ETextureFormat format, Vec2Uint texSize) {
	bool compress = (format == ETextureFormat::ERGBBC1 || format == ETextureFormat::ERGBABC6
		|| format == ETextureFormat::ERGBABC3 || format == ETextureFormat::ERGBC5);
	for (uint32_t m = 0; m < numMips; m++) {

		uint32_t mipW = std::max(1u, texSize.x >> m);
		uint32_t mipH = std::max(1u, texSize.y >> m);
		size_t mipSize = 0;

		if (compress) {
			uint32_t blockSizeW = (mipW + 3) / 4;
			uint32_t blockSizeH = (mipH + 3) / 4;

			mipExtents.push_back(Vec2Uint(blockSizeW, blockSizeH));
			mipSize = (size_t)blockSizeW * blockSizeH * TextureFormatToSize(format);
		}
		else {
			mipSize = (size_t)mipW * mipH * TextureFormatToSize(format);
		}

		mipSizes.push_back(mipSize);
		byteSize += mipSize;
	}
}

void Spike::AssetImporter::ImportTexture2D(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath, Texture2DImportDesc desc) {

	void* rawData;
	int width, height, numChannels;
	ETextureFormat stagingFormat;

	switch (desc.Format)
	{
	case ETextureFormat::ERGBA8U:
	case ETextureFormat::ERGBABC3:
	case ETextureFormat::ERGBBC1:
		rawData = stbi_load(sourcePath.string().c_str(), &width, &height, &numChannels, 4);
		stagingFormat = ETextureFormat::ERGBA8U;
		break;
	case ETextureFormat::ERGBA16F:
		rawData = stbi_load_16(sourcePath.string().c_str(), &width, &height, &numChannels, 4);
		stagingFormat = ETextureFormat::ERGBA16F;
		break;
	case ETextureFormat::ERGBA32F:
		rawData = stbi_loadf(sourcePath.string().c_str(), &width, &height, &numChannels, 4);
		stagingFormat = ETextureFormat::ERGBA32F;
		break;
	case ETextureFormat::ERGBC5:
	case ETextureFormat::ERG16F:
		rawData = stbi_load_16(sourcePath.string().c_str(), &width, &height, &numChannels, 2);
		stagingFormat = ETextureFormat::ERG16F;
		break;
	default:
		ENGINE_ERROR("Unsupported texture format for import!");
		return;
	}

	if (!rawData) {
		ENGINE_ERROR("Failed to load texture from: {}", sourcePath.string());
		return;
	}

	SUBMIT_RENDER_COMMAND(([=]() {

		Texture2DHeader header{};
		header.Width = width;
		header.Height = height;
		header.Format = desc.Format;
		header.Filter = desc.Filter;
		header.AddressU = desc.AddressU;
		header.AddressV = desc.AddressV;

		SamplerDesc samplDesc{};
		samplDesc.Filter = ESamplerFilter::EBilinear;
		samplDesc.AddressU = ESamplerAddress::EClamp;
		samplDesc.AddressV = ESamplerAddress::EClamp;
		samplDesc.AddressW = ESamplerAddress::EClamp;

		uint32_t numMips = desc.MipMap ? GetNumTextureMips(header.Width, header.Height) : 1;

		Texture2DDesc stagingTexDesc{};
		stagingTexDesc.Width = width;
		stagingTexDesc.Height = height;
		stagingTexDesc.NumMips = numMips;
		stagingTexDesc.Format = stagingFormat;
		stagingTexDesc.UsageFlags = ETextureUsageFlags::ECopySrc | ETextureUsageFlags::ECopyDst | ETextureUsageFlags::ESampled;
		stagingTexDesc.SamplerDesc = samplDesc;

		RHITexture2D* stagingTex = new RHITexture2D(stagingTexDesc);
		stagingTex->InitRHI();
		{
			RHIDevice::SubResourceCopyRegion region{ 0, 0, 0 };
			GRHIDevice->CopyDataToTexture(rawData, 0, stagingTex, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRVCompute, { region }, (size_t)stagingTexDesc.Width * stagingTexDesc.Height * TextureFormatToSize(stagingTexDesc.Format));
			stbi_image_free(rawData);

			if (desc.MipMap) {
				GRHIDevice->ImmediateSubmit([&](RHICommandBuffer* cmd) {
					GRHIDevice->MipMapTexture2D(cmd, stagingTex, EGPUAccessFlags::ESRVCompute, EGPUAccessFlags::ESRVCompute, numMips);
					});
			}
		}

		std::vector<size_t> mipSizes{};
		std::vector<Vec2Uint> mipExtents{};
		CalculateTextureMipData(mipSizes, mipExtents, header.ByteSize, numMips, header.Format, { header.Width, header.Height });

		BufferDesc mipBuffDesc{};
		mipBuffDesc.Size = header.ByteSize;
		mipBuffDesc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst;
		mipBuffDesc.MemUsage = EBufferMemUsage::ECPUOnly;

		RHIBuffer* mipBuffer = new RHIBuffer(mipBuffDesc);
		mipBuffer->InitRHI();

		bool compress = (header.Format == ETextureFormat::ERGBBC1 || header.Format == ETextureFormat::ERGBABC3 || header.Format == ETextureFormat::ERGBC5);
		if (compress) {
			CompressTexture(mipBuffer, stagingTex, mipSizes, mipExtents, 1, desc.Format);
		}
		else {

			GRHIDevice->ImmediateSubmit([&](RHICommandBuffer* cmd) {

				size_t outOffset = 0;
				GRHIDevice->BarrierTexture(cmd, stagingTex, EGPUAccessFlags::ESRVCompute, EGPUAccessFlags::ECopySrc);

				for (uint32_t m = 0; m < numMips; m++) {

					RHIDevice::SubResourceCopyRegion region{};
					region.ArrayLayer = 0;
					region.DataOffset = outOffset;
					region.MipLevel = m;

					GRHIDevice->CopyFromTextureToCPU(cmd, stagingTex, region, mipBuffer);
					GRHIDevice->BarrierBuffer(cmd, mipBuffer, mipBuffer->GetSize(), 0, EGPUAccessFlags::ECopyDst, EGPUAccessFlags::ECopyDst);

					outOffset += mipSizes[m];
				}
				});
		}

		stagingTex->ReleaseRHIImmediate();
		delete stagingTex;

		// output processed texture onto bin asset file
		{
			std::filesystem::path fullPath = GEditor->GetProjectPath() / assetPath;

			std::filesystem::create_directories(fullPath.parent_path());
			BinaryWriteStream stream(fullPath);

			if (!stream.IsOpen()) {
				ENGINE_ERROR("Failed to create asset file for texture: {}", fullPath.string());
			}
			else {
				stream << TEXTURE_2D_MAGIC << header << mipSizes;
				stream.WriteRaw(mipBuffer->GetMappedData(), mipBuffer->GetSize());
			}

			EditorRegistry::AssetInfo info{};
			info.Type = EAssetType::ETexture2D;
			info.Path = assetPath;

			GApplication->DispatchEvent<AssetImportedEvent>(info);
		}

		mipBuffer->ReleaseRHIImmediate();
		delete mipBuffer;
		}));
}

// utility
static void ImportCubeTextureInternal(RHITexture* sampledTexture, const std::filesystem::path& assetPath, CubeTextureImportDesc desc) {
	uint32_t numMips = (desc.FilterMode == ECubeTextureFilterMode::ERadiance) ? GetNumTextureMips(desc.Size, desc.Size) : 1;

	CubeTextureHeader header{};
	header.Size = desc.Size;
	header.NumMips = numMips;
	header.Format = desc.Format;
	header.Filter = desc.Filter;
	header.AddressU = desc.AddressU;
	header.AddressV = desc.AddressV;
	header.AddressW = desc.AddressW;

	CubeTextureDesc stagingTexDesc{};
	stagingTexDesc.Size = desc.Size;
	stagingTexDesc.NumMips = numMips;
	stagingTexDesc.UsageFlags = ETextureUsageFlags::ECopyDst | ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopySrc;
	stagingTexDesc.Sampler = sampledTexture->GetSampler();

	switch (desc.Format)
	{
	case ETextureFormat::ERGBA16F:
	case ETextureFormat::ERGBABC6:
		stagingTexDesc.Format = ETextureFormat::ERGBA16F;
		break;
	default:
		stagingTexDesc.Format = ETextureFormat::ERGBA32F;
		break;
	}

	RHICubeTexture* stagingTex = new RHICubeTexture(stagingTexDesc);
	stagingTex->InitRHI();

	// filter the texture
	{
		ShaderDesc shaderDesc{};
		shaderDesc.Type = EShaderType::ECompute;

		bool isNoneFilter = (desc.FilterMode == ECubeTextureFilterMode::ENone);
		shaderDesc.Name = isNoneFilter ? "CubeMapGen" : "CubeMapFiltering";

		RHIShader* shader = GShaderManager->GetShaderFromCache(shaderDesc);

		Texture2DDesc offscreenDesc{};
		offscreenDesc.Width = desc.Size;
		offscreenDesc.Height = desc.Size;
		offscreenDesc.Format = stagingTexDesc.Format;
		offscreenDesc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;
		offscreenDesc.NumMips = 1;

		RHITexture2D* offscreen = new RHITexture2D(offscreenDesc);
		offscreen->InitRHI();

		GRHIDevice->ImmediateSubmit([&](RHICommandBuffer* cmd) {

			GRHIDevice->BarrierTexture(cmd, stagingTex, EGPUAccessFlags::ENone, EGPUAccessFlags::ECopyDst);
			GRHIDevice->BarrierTexture(cmd, offscreen, EGPUAccessFlags::ENone, EGPUAccessFlags::EUAVCompute);

			RHIBindingSet* cubeSet = new RHIBindingSet(shader->GetLayouts()[0]);
			cubeSet->InitRHI();
			{
				cubeSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureUAV, offscreen->GetTextureView(), EGPUAccessFlags::EUAVCompute);
				cubeSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, sampledTexture->GetTextureView(), EGPUAccessFlags::ESRV);
				cubeSet->AddSamplerWrite(2, 0, EShaderResourceType::ESampler, sampledTexture->GetSampler());
			}

			for (int f = 0; f < 6; f++) { 
				for (uint32_t m = 0; m < numMips; m++) {

					uint32_t mipSize = desc.Size >> m;
					if (mipSize < 1) mipSize = 1;

					if (isNoneFilter) {

						CubeMapGenPushData pushData{};
						pushData.FaceIndex = f;
						pushData.OutSize = mipSize;

						GRHIDevice->BindShader(cmd, shader, { cubeSet }, &pushData);
					}
					else {

						CubeMapFilteringPushData pushData{};
						pushData.FaceIndex = f;
						pushData.FilterType = (uint8_t)desc.FilterMode;
						pushData.OutSize = mipSize;

						if (desc.FilterMode == ECubeTextureFilterMode::ERadiance) {
							pushData.Roughness = (float)m / (float)(numMips - 1);
						}

						GRHIDevice->BindShader(cmd, shader, { cubeSet }, &pushData); 
					}

					uint32_t groupCount = GetComputeGroupCount(mipSize, 8);
					GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

					// copy offscreen to our cube map
					{
						GRHIDevice->BarrierTexture(cmd, offscreen, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ECopySrc);

						RHIDevice::TextureCopyRegion srcRegion{};
						srcRegion.BaseArrayLayer = 0;
						srcRegion.LayerCount = 1;
						srcRegion.MipLevel = 0;
						srcRegion.Offset = { 0, 0, 0 };

						RHIDevice::TextureCopyRegion dstRegion{};
						dstRegion.BaseArrayLayer = f;
						dstRegion.LayerCount = 1;
						dstRegion.MipLevel = m;
						dstRegion.Offset = { 0, 0, 0 };

						GRHIDevice->CopyTexture(cmd, offscreen, srcRegion, stagingTex, dstRegion, { mipSize, mipSize });
						GRHIDevice->BarrierTexture(cmd, offscreen, EGPUAccessFlags::ECopySrc, EGPUAccessFlags::EUAVCompute);
					}
				}
			}

			GRHIDevice->BarrierTexture(cmd, stagingTex, EGPUAccessFlags::ECopyDst, EGPUAccessFlags::ESRVCompute);
			});

		offscreen->ReleaseRHIImmediate();
		delete offscreen;
	}

	std::vector<size_t> mipSizes{};
	std::vector<Vec2Uint> mipExtents{};
	CalculateTextureMipData(mipSizes, mipExtents, header.ByteSize, header.NumMips, header.Format, { header.Size, header.Size });

	// for 6 faces
	header.ByteSize *= 6;

	BufferDesc mipBuffDesc{};
	mipBuffDesc.Size = header.ByteSize;
	mipBuffDesc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst;
	mipBuffDesc.MemUsage = EBufferMemUsage::ECPUOnly;

	RHIBuffer* mipBuffer = new RHIBuffer(mipBuffDesc);
	mipBuffer->InitRHI();

	bool compress = (header.Format == ETextureFormat::ERGBABC6);
	if (compress) {
		CompressTexture(mipBuffer, stagingTex, mipSizes, mipExtents, 6, desc.Format);
	}
	else {
		GRHIDevice->ImmediateSubmit([&](RHICommandBuffer* cmd) {

			size_t outOffset = 0;
			GRHIDevice->BarrierTexture(cmd, stagingTex, EGPUAccessFlags::ESRVCompute, EGPUAccessFlags::ECopySrc);

			for (uint32_t f = 0; f < 6; f++) {
				for (uint32_t m = 0; m < numMips; m++) {

					RHIDevice::SubResourceCopyRegion region{};
					region.ArrayLayer = f;
					region.DataOffset = outOffset;
					region.MipLevel = m;

					GRHIDevice->CopyFromTextureToCPU(cmd, stagingTex, region, mipBuffer);
					GRHIDevice->BarrierBuffer(cmd, mipBuffer, mipBuffer->GetSize(), 0, EGPUAccessFlags::ECopyDst, EGPUAccessFlags::ECopyDst);

					outOffset += mipSizes[m];
				}
			}
			});
	}

	stagingTex->ReleaseRHIImmediate();
	delete stagingTex;

	// output processed texture onto bin asset file
	{
		std::filesystem::path fullPath = GEditor->GetProjectPath() / assetPath;

		std::filesystem::create_directories(fullPath.parent_path());
		BinaryWriteStream stream(fullPath);

		if (!stream.IsOpen()) {
			ENGINE_ERROR("Failed to create asset file for texture: {}", fullPath.string());
		}
		else {
			stream << CUBE_TEXTURE_MAGIC << header << mipSizes;
			stream.WriteRaw(mipBuffer->GetMappedData(), mipBuffer->GetSize());
		}

		EditorRegistry::AssetInfo info{};
		info.Type = EAssetType::ECubeTexture;
		info.Path = assetPath;

		GApplication->DispatchEvent<AssetImportedEvent>(info);
	}

	mipBuffer->ReleaseRHIImmediate();
	delete mipBuffer;
}

void Spike::AssetImporter::ImportCubeTexture(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath, CubeTextureImportDesc desc) {

	void* rawData;
	int width, height, numChannels;
	ETextureFormat stagingFormat;

	switch (desc.Format)
	{
	case ETextureFormat::ERGBA16F:
	case ETextureFormat::ERGBABC6:
		stagingFormat = ETextureFormat::ERGBA16F;
		rawData = stbi_load_16(sourcePath.string().c_str(), &width, &height, &numChannels, 4);
		break;
	case ETextureFormat::ERGBA32F:
		stagingFormat = ETextureFormat::ERGBA32F;
		rawData = stbi_loadf(sourcePath.string().c_str(), &width, &height, &numChannels, 4);
		break;
	default:
		ENGINE_ERROR("Unsupported texture format for import!");
		return;
	}

	SUBMIT_RENDER_COMMAND([=]() {

		SamplerDesc samplDesc{};
		samplDesc.Filter = ESamplerFilter::EBilinear;
		samplDesc.AddressU = ESamplerAddress::EClamp;
		samplDesc.AddressV = ESamplerAddress::EClamp;
		samplDesc.AddressW = ESamplerAddress::EClamp;

		Texture2DDesc texDesc{};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.NumMips = 1;
		texDesc.Format = stagingFormat;
		texDesc.UsageFlags = ETextureUsageFlags::ECopyDst | ETextureUsageFlags::ESampled;
		texDesc.SamplerDesc = samplDesc;

		RHITexture2D* sampledTex = new RHITexture2D(texDesc);
		sampledTex->InitRHI();
		{
			RHIDevice::SubResourceCopyRegion region{};
			region.ArrayLayer = 0;
			region.DataOffset = 0;
			region.MipLevel = 0;

			GRHIDevice->CopyDataToTexture(rawData, 0, sampledTex, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, {region}, 
				(size_t)texDesc.Width * texDesc.Height * TextureFormatToSize(texDesc.Format));
		}

		ImportCubeTextureInternal(sampledTex, assetPath, desc);
		sampledTex->ReleaseRHIImmediate();
		});
}

void Spike::AssetImporter::ImportCubeTexture(UUID sourceID, const std::filesystem::path& assetPath, CubeTextureImportDesc desc) {

	Ref<CubeTexture> sampledTex = GRegistry->LoadAsset(sourceID).As<CubeTexture>();
	RHICubeTexture* rhiTex = sampledTex->GetResource();

	SUBMIT_RENDER_COMMAND([=]() {
		ImportCubeTextureInternal(rhiTex, assetPath, desc);
		});
}