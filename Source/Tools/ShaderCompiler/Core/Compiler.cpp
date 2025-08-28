#include <Core/Compiler.h>
#include <regex>

#include <wrl/client.h>
#include <dxcapi.h>
#include <spirv_reflect.h>

#define EXIT_WITH_ERROR(...) \
COMPILER_ERROR(__VA_ARGS__); \
exit(-1);

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
		EXIT_WITH_ERROR("Failed to open: {}", path);
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
				EXIT_WITH_ERROR("Found circular dependency: {} includes itself!", name);
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
			EXIT_WITH_ERROR("Shader: {} has no entry points!", filename);
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

				for (auto& target : compTargets) {

					Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = compiler.Compile(target, sourceCode, sourcePath, fullLocalPath);

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
						EXIT_WITH_ERROR("Unexpected error! This should not happen! Unspecified compilation target");
							break;
					}
				}

				// serialize shader metadata
				{
					std::string genHeaderStr{};

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

								shader.ShaderData.IsMaterialShader = true;
								genHeaderStr += "constexpr struct " + shaderName + "MaterialResourcesStruct " + "{\n";

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
										genHeaderStr += "    uint8_t " + (*i)[2].str() + " = " + (*i)[3].str() + ";\n";
									}
								}

								genHeaderStr += "} " + shaderName + "MaterialResources;\n";
							}
						}
					}

					// parse shader resources
					if (!shader.ShaderData.IsMaterialShader) {

						auto findBindings = [&](const std::vector<char>& spirvData) {

							SpvReflectShaderModule module{};
							SpvReflectResult result = spvReflectCreateShaderModule(spirvData.size(), spirvData.data(), &module);
							if (result != SPV_REFLECT_RESULT_SUCCESS) {
								EXIT_WITH_ERROR("Unexpected error! This should not happen! Failed to create spirv reflect module");
							}

							{
								uint32_t bindingCount = 0;
								spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);

								std::vector<SpvReflectDescriptorBinding*> bindings;
								bindings.resize(bindingCount);
								spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());

								for (const auto spvB : bindings) {

									BinaryShader::ShaderMetadata::ShaderBinding b{};
									b.Binding = spvB->binding;
									b.Count = spvB->count;
									b.Set = spvB->set;

									switch (spvB->descriptor_type)
									{
									case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
										b.Type = EShaderResourceType::ETextureSRV;
										break;
									case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
										b.Type = EShaderResourceType::ETextureUAV;
										break;
									case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
										b.Type = EShaderResourceType::ESampler;
										break;
									case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
										b.Type = EShaderResourceType::EConstantBuffer;
										break;
									case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
										b.Type = spvB->resource_type & SPV_REFLECT_RESOURCE_FLAG_UAV ? EShaderResourceType::EBufferUAV : EShaderResourceType::EBufferSRV;
										break;
									default:
										b.Type = EShaderResourceType::ENone;
										break;
									}

									auto it = std::find_if(shader.ShaderData.Bindings.begin(), shader.ShaderData.Bindings.end(), [&](const auto& other) {
										return (b.Binding == other.Binding
											&& b.Count == other.Count
											&& b.Set == other.Set
											&& b.Type == other.Type);
										});

									if (it == shader.ShaderData.Bindings.end()) {
										shader.ShaderData.Bindings.push_back(b);
									}
								}
								{
									if (shader.ShaderData.PushDataSize == 0) {

										uint32_t pushDataCount = 0;
										spvReflectEnumeratePushConstantBlocks(&module, &pushDataCount, nullptr);

										if (pushDataCount > 0) {
											genHeaderStr += "struct alignas(16) " + shaderName + "PushData {\n";

											std::vector<SpvReflectBlockVariable*> pushData;
											pushData.resize(pushDataCount);
											spvReflectEnumeratePushConstantBlocks(&module, &pushDataCount, pushData.data());

											SpvReflectBlockVariable* pushBlock = pushData[0];
											for (uint32_t i = 0; i < pushBlock->member_count; i++) {

												const SpvReflectTypeDescription* tDesc = pushBlock->members[i].type_description;
												shader.ShaderData.PushDataSize += pushBlock->members[i].size;

												// TODO: support matrices where column count != row count like Mat4x3 etc... Also support Vec3 type
												genHeaderStr += "    ";
												if (tDesc->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {

													if (tDesc->traits.numeric.matrix.column_count == 2) {
														genHeaderStr += "Mat2x2 ";
													}
													else if (tDesc->traits.numeric.matrix.column_count == 3) {
														genHeaderStr += "Mat3x3 ";
													}
													else if (tDesc->traits.numeric.matrix.column_count == 4) {
														genHeaderStr += "Mat4x4 ";
													}
												}
												else if (tDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {

													if (tDesc->traits.numeric.vector.component_count == 2) {
														genHeaderStr += "Vec2 ";
													}
													else if (tDesc->traits.numeric.vector.component_count == 4) {
														genHeaderStr += "Vec4 ";
													}
												}
												else if (tDesc->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {

													if (tDesc->traits.numeric.scalar.signedness != 0) {
														genHeaderStr += "int ";
													}
													else {
														genHeaderStr += "uint32_t ";
													}
												}
												else if (tDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
													genHeaderStr += "float ";
												}
												else {
													EXIT_WITH_ERROR("Unknown or unsupported variable type in push data block! Variable index: {}", i);
												}

												genHeaderStr += pushBlock->members[i].name;
												if (tDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {

													for (uint32_t i = 0; i < tDesc->traits.array.dims_count; i++) {
														genHeaderStr += "[";
														genHeaderStr += std::to_string(tDesc->traits.array.dims[i]);
														genHeaderStr += "]";
													}
												}

												genHeaderStr += ";\n";
											}
											genHeaderStr += "};\n";
										}
									}
								}
							}
							};

						for (auto t : compTargets) {

							switch (t)
							{
							case ShaderCompiler::EShaderCompilationTarget::EVertex:
								findBindings(shader.VertexRange);
								break;
							case ShaderCompiler::EShaderCompilationTarget::EPixel:
								findBindings(shader.PixelRange);
								break;
							case ShaderCompiler::EShaderCompilationTarget::ECompute:
								findBindings(shader.ComputeRange);
								break;
							default:
								EXIT_WITH_ERROR("Unexpected error! This should not happen! Unspecified compilation target")
								break;
							}
						}
					}

					if (genHeaderStr.size() > 0) {

						std::filesystem::create_directories(genPath);
						std::string genHeaderName = shaderName + ".h";
						std::ofstream genHeader(GetFilePath(genPath, genHeaderName), std::ios::out | std::ios::trunc);

						genHeader << genHeaderStr;
						genHeader.close();
					}
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
			EXIT_WITH_ERROR("Unexpected error! This should not happen! Failed to create dxc utils instance")
		}

		hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
		if (FAILED(hres)) {
			EXIT_WITH_ERROR("Unexpected error! This should not happen! Failed to create dxc compiler instance")
		}

		hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
		if (FAILED(hres)) {
			EXIT_WITH_ERROR("Unexpected error! This should not happen! Failed to create dxc library instance")
		}

		hres = Library->CreateIncludeHandler(&IncludeHandler);
		if (FAILED(hres)) {
			EXIT_WITH_ERROR("Unexpected error! This should not happen! Failed to create dxc include handler instance")
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
			EXIT_WITH_ERROR("Unexpected error! This should not happen! Unspecified compilation target")
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
				EXIT_WITH_ERROR("Compilation failed: {}", (const char*)errorBlob->GetBufferPointer())
			}
		}

		Microsoft::WRL::ComPtr<IDxcBlob> shaderObj;
		result->GetResult(&shaderObj);

		return shaderObj;
	}
}