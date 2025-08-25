constexpr struct SSAOResourcesStruct {
    uint8_t SampledTextureA = 0;
    uint8_t SampledTextureB = 8;
    uint8_t NoiseMap = 16;
    uint8_t OutTexture = 24;
    uint8_t Radius = 28;
    uint8_t Bias = 32;
    uint8_t NumSamples = 36;
    uint8_t Intensity = 40;
    uint8_t SSAOStage = 44;
    uint8_t TexSize = 48;
} SSAOResources;
