#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec4 fragTangent;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;
layout(location = 4) in flat uint matBufferIndex;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;

layout(set = 1, binding = 0) uniform sampler2D Textures[];

layout(set = 1, binding = 1) readonly buffer DataBuffer {

    vec4 ColorData[16];
    float ScalarData[48];

    uint TexIndex[16];

} DataBuffers[];

#define IndexInvalid 10000000

vec4 GetTextureValue(uint valIndex, vec2 UV) {

    uint texIndex = DataBuffers[nonuniformEXT(matBufferIndex)].TexIndex[valIndex];

    if (texIndex == IndexInvalid)
        return vec4(1.0, 0.0, 1.0, 1.0);

    vec4 value = texture(Textures[nonuniformEXT(texIndex)], UV);

    return value;
}

vec4 GetColorValue(uint valIndex) {

    vec4 value = DataBuffers[nonuniformEXT(matBufferIndex)].ColorData[valIndex];

    return value;
}

float GetScalarValue(uint valIndex) {

    float value = DataBuffers[nonuniformEXT(matBufferIndex)].ScalarData[valIndex];

    return value;
}

#define ALBEDO_MAP 0
#define NORMAL_MAP 1
#define METTALIC_MAP 3
#define ROUGHNESS_MAP 4
#define AO_MAP 2

void main() 
{
    vec4 albedo = GetTextureValue(ALBEDO_MAP, fragTexCoord);
    vec3 normalSample = GetTextureValue(NORMAL_MAP, fragTexCoord).rgb;

    float ao = GetTextureValue(AO_MAP, fragTexCoord).r;
    float metallic = GetTextureValue(METTALIC_MAP, fragTexCoord).r;
    float roughness = GetTextureValue(ROUGHNESS_MAP, fragTexCoord).r;

    if (albedo.a < 0.8) {

        discard;
    }

    vec3 N;
    if(fragTangent.xyz == vec3(0, 0, 0) || normalSample == vec3(1, 1, 1)) {
        N = normalize(fragNormal); 
    } else {

        vec3 B = cross(fragNormal, fragTangent.xyz) * fragTangent.w;
        mat3 fragTBN = mat3(fragTangent.xyz, B, fragNormal);

        N = normalize(fragTBN*normalize(normalSample*2.0 - 1.0));
    }
   
    outAlbedo = albedo;
    outNormal = vec4(N, 1.0);
    outMaterial = vec4(roughness, metallic, ao, 1.0);
}
