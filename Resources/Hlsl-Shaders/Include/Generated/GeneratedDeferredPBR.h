constexpr struct DeferredPBRResourcesStruct {
    uint8_t SceneObjectsBuffer = 0;
} DeferredPBRResources;
constexpr struct DeferredPBRMaterialResourcesStruct {
    uint8_t AlbedoMap = 0;
    uint8_t NormalMap = 1;
    uint8_t MettalicMap = 2;
    uint8_t RoughnessMap = 3;
    uint8_t AOMap = 4;
} DeferredPBRMaterialResources;
