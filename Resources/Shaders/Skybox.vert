#version 450

layout(location = 0) out vec3 viewDir;
layout(location = 1) out vec2 texCoord;

layout(set = 0, binding = 0) uniform SceneData {

    mat4 View;
    mat4 Proj;
    mat4 ViewProj;
    mat4 InverseProj;
    mat4 InverseView;

    vec3 CameraPos;
    float AmbientIntensity;

} SceneDataBuffer;


void main() {

    texCoord = vec2((gl_VertexIndex << 1) & 2, (gl_VertexIndex & 2));

	gl_Position = vec4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);

	viewDir = mat3(SceneDataBuffer.InverseView) * (SceneDataBuffer.InverseProj * gl_Position).xyz;
}