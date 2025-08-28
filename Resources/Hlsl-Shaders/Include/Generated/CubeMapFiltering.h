struct alignas(16) CubeMapFilteringPushData {
    uint32_t FaceIndex;
    uint32_t FilterType;
    uint32_t OutSize;
    float Roughness;
};
