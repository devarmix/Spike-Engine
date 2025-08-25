constexpr struct BloomResourcesStruct {
    uint8_t SrcSize = 0;
    uint8_t OutSize = 8;
    uint8_t MipLevel = 16;
    uint8_t Threadshold = 20;
    uint8_t SoftThreadshold = 24;
    uint8_t SampledTextureA = 28;
    uint8_t SampledTextureB = 36;
    uint8_t OutTexture = 44;
    uint8_t BloomStage = 48;
    uint8_t BloomIntensity = 52;
    uint8_t FilterRadius = 56;
} BloomResources;
