#version 450

#extension GL_GOOGLE_include_directive : require

#include "SpikeCommon.glsl"

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTangent;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragColor;
layout(location = 4) out mat3 fragTBN;

void main() {

    Vertex v = PushConstants.VertexBuffer.Vertices[gl_VertexIndex];
    
    vec4 fragPos = PushConstants.RenderMatrix * vec4(v.Position.xyz, 1.0);
    
    gl_Position = SceneDataBuffer.ViewProj * fragPos;

    fragTexCoord = vec2(v.Position.w, v.Normal.w);

    mat4 transposeInverseModel = transpose(inverse(PushConstants.RenderMatrix));

    fragTangent = normalize(vec3(transposeInverseModel * vec4(v.Tangent.xyz, 0.0)));

    fragNormal = normalize(vec3(transposeInverseModel * vec4(v.Normal.xyz, 0.0)));

    fragTangent = normalize(fragTangent - dot(fragTangent, fragNormal) * fragNormal);

    vec3 B = cross(fragNormal, fragTangent) * v.Tangent.w;

    fragTBN = mat3(fragTangent, B, fragNormal);

    fragColor = v.Color;
}