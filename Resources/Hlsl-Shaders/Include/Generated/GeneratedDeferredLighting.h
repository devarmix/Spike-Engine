constexpr struct DeferredLightingResourcesStruct {
    uint8_t AlbedoMap = 0;
    uint8_t NormalMap = 8;
    uint8_t MaterialMap = 16;
    uint8_t DepthMap = 24;
    uint8_t EnvMap = 32;
    uint8_t IrrMap = 40;
    uint8_t BRDFMap = 48;
    uint8_t LightsBuffer = 56;
    uint8_t EnvMapNumMips = 60;
} DeferredLightingResources;
