#include <fstream>
#include <string>
#include <vector>

namespace ShaderCompiler {

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

            uint8_t SizeOfResources;
            uint8_t UsesSceneData;
        } ShaderData;

        void Serialize(std::ofstream& stream) const;
        void Deserialize(std::ifstream& stream);
    };

    bool ValidateBinary(std::ifstream& stream);
}