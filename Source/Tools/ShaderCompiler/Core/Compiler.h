#pragma once

#include <Core/Log.h>
#include <CompilerInclude.h>
#include <filesystem>
#include <vector>

#include <wrl/client.h>
#include <dxc/dxcapi.h>

namespace ShaderCompiler {

    enum class EShaderCompilationTarget : uint8_t {

        ENone = 0,
        EVertex,
        EPixel,
        ECompute
    };

    struct DXCWrapper {

        DXCWrapper();
        Microsoft::WRL::ComPtr<IDxcBlob> Compile(EShaderCompilationTarget compTarget, const std::string& sourceCode, const std::filesystem::path& includePath, const std::filesystem::path& fullLocalPath);
        Microsoft::WRL::ComPtr<IDxcBlob> PreProcess(const std::string& sourceCode, const std::filesystem::path& includePath, const std::filesystem::path& fullLocalPath);

        Microsoft::WRL::ComPtr<IDxcUtils> Utils{};
        Microsoft::WRL::ComPtr<IDxcCompiler3> Compiler{};
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> IncludeHandler{};
        Microsoft::WRL::ComPtr<IDxcLibrary> Library{};
    };

	void CompileShaders(const std::filesystem::path& sourcePath, const std::filesystem::path& cachePath, const std::filesystem::path& genPath);

    std::string LoadTextFile(const std::filesystem::path& path);
    bool LoadTextFile(const std::filesystem::path& path, std::string& out);

    template<typename T>
    void IterateAllFilesInDirectory(const std::filesystem::path& path, T&& iterator) {

        for (const auto& entry : std::filesystem::directory_iterator(path)) {

            std::filesystem::path filename = entry.path().filename();
            if (entry.is_directory()) {
                IterateAllFilesInDirectory(entry.path(), iterator);
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