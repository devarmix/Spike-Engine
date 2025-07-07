#version 450

layout(location = 0) out vec3 viewDir;

struct SceneDrawData {

	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	mat4 InverseProj;
	mat4 InverseView;

	vec4 CameraPos;

	float AmbientIntensity;
	float NearProj;
	float FarProj;

	float pad0;
};

#define MAX_SCENE_DRAWS_PER_FRAME 8

layout(std140, set = 0, binding = 0) uniform DeclSceneDataBuffer {

	SceneDrawData SceneData[MAX_SCENE_DRAWS_PER_FRAME];
} SceneDataBuffer;

struct SkyboxData {

	uint SceneDataOffset;
	float pad0[3];
};

layout(push_constant) uniform Constants {

    SkyboxData SkyboxConstants;
};


void main() {

    vec2 texCoord = vec2((gl_VertexIndex << 1) & 2, (gl_VertexIndex & 2));

	gl_Position = vec4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);

    mat4 invView = SceneDataBuffer.SceneData[SkyboxConstants.SceneDataOffset].InverseView;
    mat4 invProj = SceneDataBuffer.SceneData[SkyboxConstants.SceneDataOffset].InverseProj;

	viewDir = mat3(invView) * (invProj * gl_Position).xyz;
}