#pragma once

#include <Core/Log.h>
#include <CompilerInclude.h>
#include <filesystem>
#include <vector>

#include <wrl/client.h>
#include <dxcapi.h>

namespace ShaderCompiler {

    enum class EShaderCompilationTarget : uint8_t {

        ENone = 0,
        EVertex,
        EPixel,
        ECompute
    };

    struct DXCWrapper {

        DXCWrapper();
        Microsoft::WRL::ComPtr<IDxcBlob> Compile(EShaderCompilationTarget compTarget, const std::string& sourceCode, const std::string& includePath, const std::string& fullLocalPath);

        Microsoft::WRL::ComPtr<IDxcUtils> Utils{};
        Microsoft::WRL::ComPtr<IDxcCompiler3> Compiler{};
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> IncludeHandler{};
        Microsoft::WRL::ComPtr<IDxcLibrary> Library{};
    };

	void CompileShaders(const std::string& sourcePath, const std::string& cachePath, const std::string& genPath);

	size_t ComputeShaderHash(const std::string& sourcePath, const std::string& includePath, const std::string& filename, std::vector<std::string>& allFilenames);

    std::string LoadTextFile(const std::string& path);
    bool LoadTextFile(const std::string& path, std::string& out);

    template<typename T>
    void IterateAllFilesInDirectory(const std::string& path, T&& iterator) {

        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(path))) {

            std::filesystem::path filename = entry.path().filename();
            if (entry.is_directory()) {
                IterateAllFilesInDirectory(entry.path().string(), iterator);
            }
            else if (entry.is_regular_file()) {
                if (filename.extension().string() == ".hlsl")
                    iterator(path, filename.string(), filename.stem().string());
            }
            else
                COMPILER_WARN("Found unknown file: {}", filename.string());
        }
    }
}