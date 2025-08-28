struct alignas(16) IndirectCullPushData {
    uint32_t IsPrepass;
    uint32_t PyramidSize;
    float CullZNear;
    float Padding0;
};
