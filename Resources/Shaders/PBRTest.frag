#version 450

#extension GL_GOOGLE_include_directive : require

#include "SpikeCommon.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragTangent;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;
layout(location = 4) in mat3 fragTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;

#define AlbedoMap     0
#define NormalMap     1
#define AOMap         2
#define MetallicMap   3
#define RoughnessMap  4

void main() {

    vec4 albedo = vec4(1.0);
   // vec3 normalSample = GetTextureValue(NormalMap, fragTexCoord).rgb;

    float ao = 1.0;
    float metallic = 0.4;
    float roughness = 0.9;

    if (albedo.a < 0.5) {

        discard;
    }

    //vec3 N;
    //if(fragTangent == vec3(0, 0, 0) || normalSample == vec3(1, 1, 1)) {
    //    N = normalize(fragNormal); 
    //} else {
    //    N = normalize(fragTBN*normalize(normalSample*2.0 - 1.0));
    //}
   
    outAlbedo = albedo;
    outNormal = vec4(fragNormal, 1.0);
    outMaterial = vec4(roughness, metallic, ao, 1.0);
}
