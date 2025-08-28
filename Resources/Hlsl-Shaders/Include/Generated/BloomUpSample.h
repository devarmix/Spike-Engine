struct alignas(16) BloomUpSamplePushData {
    Vec2 OutSize;
    uint32_t BloomStage;
    float Intensity;
    float FilterRadius;
    float Padding0[3];
};
