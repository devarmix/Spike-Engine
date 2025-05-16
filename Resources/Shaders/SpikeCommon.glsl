#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform SceneData {

    mat4 View;
    mat4 Proj;
    mat4 ViewProj;
    mat4 InverseProj;
    mat4 InverseView;

    vec3 CameraPos;
    float AmbientIntensity;

} SceneDataBuffer;

layout(set = 1, binding = 0) uniform sampler2D Textures[];

layout(set = 1, binding = 1) readonly buffer DataBuffer {

    vec4 ColorData[16];
    float ScalarData[48];

    uint TexIndex[16];

} DataBuffers[];


struct Vertex {

    vec4 Position;           // w - UV_x
    vec4 Normal;             // w - UV_y
    vec4 Color;
    vec4 Tangent;            // w - handedness
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {

    Vertex Vertices[];
};

layout( push_constant ) uniform Constants {

    uint BufIndex;

    mat4 RenderMatrix;
    VertexBuffer VertexBuffer;

} PushConstants;


#define IndexInvalid 10000000

vec4 GetTextureValue(uint valIndex, vec2 UV) {

    uint bufIndex = PushConstants.BufIndex;
    uint texIndex = DataBuffers[nonuniformEXT(bufIndex)].TexIndex[valIndex];

    if (texIndex == IndexInvalid)
        return vec4(1.0, 0.0, 1.0, 1.0);

    vec4 value = texture(Textures[nonuniformEXT(texIndex)], UV);

    return value;
}

vec4 GetColorValue(uint valIndex) {

    uint bufIndex = PushConstants.BufIndex;
    vec4 value = DataBuffers[nonuniformEXT(bufIndex)].ColorData[valIndex];

    return value;
}

float GetScalarValue(uint valIndex) {

    uint bufIndex = PushConstants.BufIndex;
    float value = DataBuffers[nonuniformEXT(bufIndex)].ScalarData[valIndex];

    return value;
}