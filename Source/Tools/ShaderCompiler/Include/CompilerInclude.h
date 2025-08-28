#include <fstream>
#include <string>
#include <vector>

namespace ShaderCompiler {

    // copy from Engine/Renderer/Shader.h
    enum class EShaderResourceType : uint8_t {

        ENone = 0,
        ETextureSRV,
        ETextureUAV,
        EBufferSRV,
        EBufferUAV,
        EConstantBuffer,
        ESampler
    };

    struct BinaryShader {

        std::vector<char> VertexRange;
        std::vector<char> PixelRange;
        std::vector<char> ComputeRange;

        size_t Hash;

        struct MaterialMetadata {

            std::vector<std::string> PScalar;
            std::vector<std::string> PUint;
            std::vector<std::string> PVec2;
            std::vector<std::string> PVec4;
            std::vector<std::string> PTexture;
        } MaterialData;

        struct ShaderMetadata {

            struct ShaderBinding {

                EShaderResourceType Type;
                uint32_t Binding;
                uint32_t Count;
                uint32_t Set;
            };

            std::vector<ShaderBinding> Bindings;
            uint32_t PushDataSize;

            bool IsMaterialShader = false;
        } ShaderData;

        void Serialize(std::ofstream& stream) const;
        void Deserialize(std::ifstream& stream);
    };

    bool ValidateBinary(std::ifstream& stream);
}