#version 450

#extension GL_GOOGLE_include_directive : require

#include "DrawCommon.glsl"

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec4 fragTangent;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragColor;
layout(location = 4) out flat uint matBufferIndex;

struct MaterialPushConstants {

	uint SceneDataOffset;
	float pad0[3];
};

layout(push_constant) uniform Constants {

    MaterialPushConstants PushConstants;
};

void main()
{
    uint instID = gl_InstanceIndex;

    uint indexOffset = MetadataBuffer.ObjectsMetadata[instID].FirstIndex;
    uint vIndex = MetadataBuffer.ObjectsMetadata[instID].IndexBufferAddress.Indices[gl_VertexIndex + indexOffset];

    Vertex v = MetadataBuffer.ObjectsMetadata[instID].VertexBufferAddress.Vertices[vIndex];

    //vec3 pos = cubePositions[gl_VertexIndex];

    vec4 fragPos = MetadataBuffer.ObjectsMetadata[instID].GlobalTransform * vec4(v.Position.xyz, 1.0);
    gl_Position = SceneDataBuffer.SceneData[PushConstants.SceneDataOffset].ViewProj * fragPos;

    fragTexCoord = vec2(v.Position.w, v.Normal.w);

    mat4 transposeInverseTransform = transpose(inverse(MetadataBuffer.ObjectsMetadata[instID].GlobalTransform));

    vec3 tangent = normalize(vec3(transposeInverseTransform * vec4(v.Tangent.xyz, 0.0)));
    fragNormal = normalize(vec3(transposeInverseTransform * vec4(v.Normal.xyz, 0.0)));
    tangent = normalize(tangent - dot(tangent, fragNormal) * fragNormal);

    fragTangent = vec4(tangent, v.Tangent.w);
    fragColor = v.Color;

    matBufferIndex = MetadataBuffer.ObjectsMetadata[instID].MaterialBufferIndex;
}