#include <Core/Compiler.h>
#include <regex>

#include <wrl/client.h>
#include <dxcapi.h>

// utility
void HashCombine(size_t& hash1, size_t hash2) {
	hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
}

std::string GetFilePath(const std::string& path, const std::string& filename) {
	return path + "/" + filename;
}

bool ShaderCompiler::LoadTextFile(const std::string& path, std::string& out) {

	std::ifstream stream(path);

	if (stream.is_open()) {

		std::stringstream ss;
		ss << stream.rdbuf();
		stream.close();

		out = ss.str();
		return true;
	}

	return false;
}

std::string ShaderCompiler::LoadTextFile(const std::string& path) {

	std::string out;
	if (!LoadTextFile(path, out)) {

		COMPILER_ERROR("Failed to open: {}", path);
		exit(-1);
	}

	return out;
}

size_t ShaderCompiler::ComputeShaderHash(const std::string& path, const std::string& includePath, const std::string& filename, std::vector<std::string>& allFilenames) {

	std::string fileSource;
	if (!LoadTextFile(GetFilePath(includePath, filename), fileSource)) {
		fileSource = LoadTextFile(GetFilePath(path, filename));
	}

	size_t hash = std::hash<std::string>{}(fileSource);

	std::regex pattern(R"(#include\s+\"([^"]+)\")");
	auto begin = std::sregex_iterator(fileSource.begin(), fileSource.end(), pattern);

	for (auto& i = begin; i != std::sregex_iterator(); i++) {
		std::string name = (*i)[1].str();
		{
			auto it = std::find(allFilenames.begin(), allFilenames.end(), name);
			if (it != allFilenames.end()) {
				COMPILER_ERROR("Found circular dependency: {} includes itself!", name);
				exit(-1);
			}
			allFilenames.push_back(name);
		}
		HashCombine(hash, ComputeShaderHash(path, includePath, (*i)[1].str(), allFilenames));
	}

	return hash;
}

void ShaderCompiler::CompileShaders(const std::string& sourcePath, const std::string& cachePath, const std::string& genPath) {

	DXCWrapper compiler = DXCWrapper();

	IterateAllFilesInDirectory(sourcePath, [&](const std::string& path, const std::string& filename, const std::string& shaderName) {
		COMPILER_TRACE("Parsing shader: {}", filename);

		std::vector<std::string> allFilenames = { filename };
		size_t hash = ComputeShaderHash(path, sourcePath, filename, allFilenames);

		std::string fullLocalPath = GetFilePath(path, filename);
		std::string sourceCode = LoadTextFile(fullLocalPath);
		std::vector<EShaderCompilationTarget> compTargets;
		{
			std::smatch match;
			{
				std::regex pattern(R"(CSMain\s*\([^)]*\))");
				if (std::regex_search(sourceCode, match, pattern)) {
					compTargets.push_back(EShaderCompilationTarget::ECompute);
				}
			}
			{
				std::regex pattern(R"(VSMain\s*\([^)]*\))");
				if (std::regex_search(sourceCode, match, pattern)) {
					compTargets.push_back(EShaderCompilationTarget::EVertex);
				}
			}
			{
				std::regex pattern(R"(PSMain\s*\([^)]*\))");
				if (std::regex_search(sourceCode, match, pattern)) {
					compTargets.push_back(EShaderCompilationTarget::EPixel);
				}
			}
		}

		if (compTargets.size() == 0) {
			COMPILER_ERROR("Shader: {} has no entry points!", filename);
			exit(-1);
		}

		{
			std::string binaryName = shaderName + ".bin";

			bool needsCompilation = true;
			std::ifstream rFile(GetFilePath(cachePath, binaryName), std::ios::binary);
			if (rFile.is_open()) {

				if (!ValidateBinary(rFile)) {
					COMPILER_ERROR("Corrupted cached shader file! This will trigger recompilation and should not happen");
				}
				else {
					BinaryShader shader{};
					shader.Deserialize(rFile);

					if (shader.Hash == hash) needsCompilation = false;
				}
				rFile.close();
			}

			if (needsCompilation) {
				std::filesystem::create_directories(cachePath);
				std::ofstream wFile(GetFilePath(cachePath, binaryName), std::ios::binary | std::ios::out | std::ios::trunc);

				const uint32_t MAGIC_ID = 'SCBF';
				wFile.write((char*)&MAGIC_ID, sizeof(uint32_t));

				BinaryShader shader{};
				shader.Hash = hash;

				std::vector<Microsoft::WRL::ComPtr<IDxcBlob>> shaderBlobs;
				for (auto& target : compTargets) {

					Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = compiler.Compile(target, sourceCode, sourcePath, fullLocalPath);
					shaderBlobs.push_back(shaderBlob);

					size_t sizeOfBlob = shaderBlob->GetBufferSize();
					switch (target)
					{
					case ShaderCompiler::EShaderCompilationTarget::EVertex:
						shader.VertexRange.resize(sizeOfBlob);
						memcpy(shader.VertexRange.data(), shaderBlob->GetBufferPointer(), sizeOfBlob);
						break;
					case ShaderCompiler::EShaderCompilationTarget::EPixel:
						shader.PixelRange.resize(sizeOfBlob);
						memcpy(shader.PixelRange.data(), shaderBlob->GetBufferPointer(), sizeOfBlob);
						break;
					case ShaderCompiler::EShaderCompilationTarget::ECompute:
						shader.ComputeRange.resize(sizeOfBlob);
						memcpy(shader.ComputeRange.data(), shaderBlob->GetBufferPointer(), sizeOfBlob);
						break;
					default:
						COMPILER_ERROR("Unexpected error!This should not happen! Unspecified compilation target");
						exit(-1);
						break;
					}
				}

				// serialize shader metadata
				{
					std::filesystem::create_directories(genPath);

					std::string genHeaderName = "Generated" + shaderName + ".h";
					std::ofstream genHeader(GetFilePath(genPath, genHeaderName), std::ios::out | std::ios::trunc);
					//genHeader << "Hello" << "\n";

					// parse shader resources
					{
						std::string::const_iterator resBegin;
						std::string::const_iterator resEnd;
						{
							std::regex pattern(R"(BEGIN_DECL_SHADER_RESOURCES)");
							std::smatch match;

							if (std::regex_search(sourceCode, match, pattern)) {
								resBegin = match[0].second;
							}
							else {
								resBegin = sourceCode.end();
							}
						}
						if (resBegin != sourceCode.end()) {

							{
								std::regex pattern(R"(END_DECL_SHADER_RESOURCES)");
								std::smatch match;

								if (std::regex_search(sourceCode, match, pattern)) {
									resEnd = match[0].first;
								}
								else {
									resEnd = sourceCode.end();
								}
							}
							if (resEnd != sourceCode.end()) {
								genHeader << "constexpr struct " << shaderName << "ResourcesStruct " << "{\n";

								std::map<std::string, size_t> typeSizes = {
									{"DECL_SHADER_SCALAR", sizeof(float)},
									{"DECL_SHADER_UINT", sizeof(uint32_t)},
									{"DECL_SHADER_FLOAT2", sizeof(float) * 2},
									{"DECL_SHADER_FLOAT4", sizeof(float) * 4},
									{"DECL_SHADER_MATRIX2x2", sizeof(float) * 4},
									{"DECL_SHADER_MATRIX4x4", sizeof(float) * 16},
									{"DECL_SHADER_TEXTURE_SRV", sizeof(uint32_t) * 2},
									{"DECL_SHADER_TEXTURE_UAV", sizeof(uint32_t)},
									{"DECL_SHADER_BUFFER_SRV", sizeof(uint32_t)},
									{"DECL_SHADER_BUFFER_UAV", sizeof(uint32_t)}
								};

								{
									std::regex pattern(R"((DECL_SHADER_SCALAR|DECL_SHADER_UINT|DECL_SHADER_FLOAT2|DECL_SHADER_FLOAT4|DECL_SHADER_MATRIX2x2|DECL_SHADER_MATRIX4x4|DECL_SHADER_TEXTURE_SRV|DECL_SHADER_TEXTURE_UAV|DECL_SHADER_BUFFER_SRV|DECL_SHADER_BUFFER_UAV)\s*\(\s*(\w+)\s*\))");
									auto begin = std::sregex_iterator(resBegin, resEnd, pattern);
									for (auto& i = begin; i != std::sregex_iterator(); i++) {
										genHeader << "    uint8_t " << (*i)[2].str() << " = " << std::to_string(shader.ShaderData.SizeOfResources) << ";\n";
										shader.ShaderData.SizeOfResources += (uint8_t)typeSizes[(*i)[1].str()];
									}
								}
								{
									std::regex pattern(R"(DECL_SHADER_RESOURCES_STRUCT_PADDING\s*\(\s*(\d+)\s*\))");
									auto begin = std::sregex_iterator(resBegin, resEnd, pattern);

									for (auto& i = begin; i != std::sregex_iterator(); i++) {
										shader.ShaderData.SizeOfResources += uint8_t(std::stoi((*i)[1].str()) * sizeof(float));
									}
								}
								if (shader.ShaderData.SizeOfResources % 16 != 0) {
									COMPILER_ERROR("Shader: {0} resources size is not multiple of 16, size: {1}", filename, shader.ShaderData.SizeOfResources);
									exit(-1);
								}

								//genHeader << "    ESizeOfAll = " << std::to_string(sizeOfResources) << "\n";
								genHeader << "} " << shaderName << "Resources;\n";
							}
						}
					}

					// parse material resources
					{
						std::string::const_iterator resBegin;
						std::string::const_iterator resEnd;
						{
							std::regex pattern(R"(BEGIN_DECL_MATERIAL_RESOURCES)");
							std::smatch match;

							if (std::regex_search(sourceCode, match, pattern)) {
								resBegin = match[0].second;
							}
							else {
								resBegin = sourceCode.end();
							}
						}
						if (resBegin != sourceCode.end()) {

							{
								std::regex pattern(R"(END_DECL_MATERIAL_RESOURCES)");
								std::smatch match;

								if (std::regex_search(sourceCode, match, pattern)) {
									resEnd = match[0].first;
								}
								else {
									resEnd = sourceCode.end();
								}
							}
							if (resEnd != sourceCode.end()) {
								genHeader << "constexpr struct " << shaderName << "MaterialResourcesStruct " << "{\n";

								std::map<std::string, std::vector<std::string>*> paramStorages = {

									{"DECL_MATERIAL_SCALAR", &shader.MaterialData.PScalar},
									{"DECL_MATERIAL_UINT", &shader.MaterialData.PUint},
									{"DECL_MATERIAL_FLOAT2", &shader.MaterialData.PVec2},
									{"DECL_MATERIAL_FLOAT4", &shader.MaterialData.PVec4},
									{"DECL_MATERIAL_TEXTURE_2D_SRV", &shader.MaterialData.PTexture}
								};
								{
									std::regex pattern(R"((DECL_MATERIAL_SCALAR|DECL_MATERIAL_UINT|DECL_MATERIAL_FLOAT2|DECL_MATERIAL_FLOAT4|DECL_MATERIAL_TEXTURE_2D_SRV)\s*\(\s*(\w+)\s*,\s*(\d+)\s*\))");
									auto begin = std::sregex_iterator(resBegin, resEnd, pattern);

									for (auto& i = begin; i != std::sregex_iterator(); i++) {

										paramStorages[(*i)[1].str()]->push_back((*i)[2].str());
										genHeader << "    uint8_t " << (*i)[2].str() << " = " << std::stoi((*i)[3].str()) << ";\n";
									}
								}

								genHeader << "} " << shaderName << "MaterialResources;\n";
							}
						}
					}

					// parse additional data
					{
						std::regex pattern(R"(USE_SCENE_RENDERING_DATA|USE_MATERIAL_RENDERING_DATA)");
						std::smatch match;

						if (std::regex_search(sourceCode, match, pattern)) {
							shader.ShaderData.UsesSceneData = 1;
						}
					}

					genHeader.close();
				}

				shader.Serialize(wFile);
				wFile.close();

				COMPILER_INFO("Shader: {} was successfully compiled", filename);
			}
		}
		});
}

namespace ShaderCompiler {

	DXCWrapper::DXCWrapper() {

		HRESULT hres{};

		hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(Utils.ReleaseAndGetAddressOf()));
		if (FAILED(hres)) {
			COMPILER_ERROR("Unexpected error! This should not happen! Failed to create dxc utils instance");
			exit(-1);
		}

		hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
		if (FAILED(hres)) {
			COMPILER_ERROR("Unexpected error! This should not happen! Failed to create dxc compiler instance");
			exit(-1);
		}

		hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
		if (FAILED(hres)) {
			COMPILER_ERROR("Unexpected error! This should not happen! Failed to create dxc library instance");
			exit(-1);
		}

		hres = Library->CreateIncludeHandler(&IncludeHandler);
		if (FAILED(hres)) {
			COMPILER_ERROR("Unexpected error! This should not happen! Failed to create dxc include handler instance");
			exit(-1);
		}
	}

	Microsoft::WRL::ComPtr<IDxcBlob> DXCWrapper::Compile(EShaderCompilationTarget compTarget, const std::string& sourceCode, const std::string& includePath, const std::string& fullLocalPath) {

		DxcBuffer srcBuffer = {

			.Ptr = &*sourceCode.begin(),
			.Size = (uint32_t)sourceCode.size(),
			.Encoding = 0
		};

		LPCWSTR targetProfile{};
		LPCWSTR targetEntryPoint{};
		switch (compTarget)
		{
		case ShaderCompiler::EShaderCompilationTarget::EVertex:
			targetProfile = L"vs_6_4";
			targetEntryPoint = L"VSMain";
			break;
		case ShaderCompiler::EShaderCompilationTarget::EPixel:
			targetProfile = L"ps_6_4";
			targetEntryPoint = L"PSMain";
			break;
		case ShaderCompiler::EShaderCompilationTarget::ECompute:
			targetProfile = L"cs_6_4";
			targetEntryPoint = L"CSMain";
			break;
		default:
			COMPILER_ERROR("Unexpected error! This should not happen! Unspecified compilation target");
			exit(-1);
			break;
		}

		std::wstring wincludePath(includePath.begin(), includePath.end());
		std::wstring wfilePath(fullLocalPath.begin(), fullLocalPath.end());
		std::vector<LPCWSTR> args = {

			L"-Zpc",
			L"-HV",
			L"2021",
			L"-T",
			targetProfile,
			L"-E",
			targetEntryPoint,
			L"-I",
			wincludePath.c_str(),
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			wfilePath.c_str()
		};

		HRESULT hres{};

		Microsoft::WRL::ComPtr<IDxcResult> result;
		hres = Compiler->Compile(&srcBuffer, args.data(), (uint32_t)args.size(), IncludeHandler.Get(), IID_PPV_ARGS(&result));

		if (SUCCEEDED(hres))
			result->GetStatus(&hres);

		if (FAILED(hres) && (result)) {
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
			hres = result->GetErrorBuffer(&errorBlob);
			if (SUCCEEDED(hres) && errorBlob) {
				COMPILER_ERROR("Compilation failed: {}", (const char*)errorBlob->GetBufferPointer());
				exit(-1);
			}
		}

		Microsoft::WRL::ComPtr<IDxcBlob> shaderObj;
		result->GetResult(&shaderObj);

		return shaderObj;
	}
}