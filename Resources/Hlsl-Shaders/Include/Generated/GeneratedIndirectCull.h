constexpr struct IndirectCullResourcesStruct {
    uint8_t DrawCommandsBuffer = 0;
    uint8_t DrawCountsBuffer = 4;
    uint8_t BatchOffsetsBuffer = 8;
    uint8_t LastVisibilityBuffer = 12;
    uint8_t SceneObjectsBuffer = 16;
    uint8_t CurrentVisibilityBuffer = 20;
    uint8_t DepthPyramid = 24;
    uint8_t IsPrepass = 32;
    uint8_t PyramidSize = 36;
    uint8_t CullZNear = 40;
} IndirectCullResources;
