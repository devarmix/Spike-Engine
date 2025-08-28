struct alignas(16) BloomDownSamplePushData {
    Vec2 SrcSize;
    Vec2 OutSize;
    float Threadshold;
    float SoftThreadshold;
    uint32_t MipLevel;
    float Padding0;
};
