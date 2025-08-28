struct alignas(16) SSAOGenPushData {
    float Radius;
    float Bias;
    float Intensity;
    uint32_t NumSamples;
    Vec2 TexSize;
    float Padding0[2];
};
