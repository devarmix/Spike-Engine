#extension GL_EXT_buffer_reference : require

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

struct Vertex {

    vec4 Position;           // w - UV_x
    vec4 Normal;             // w - UV_y
    vec4 Color;
    vec4 Tangent;            // w - handedness
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {

    Vertex Vertices[];
};

layout(buffer_reference, std430) readonly buffer IndexBuffer {

    uint Indices[];
};

struct SceneObjectMetadata {

	vec4 BoundsOrigin; // w - bounds radius
	vec4 BoundsExtents;
	mat4 GlobalTransform;

	uint FirstIndex;
	uint IndexCount;

	IndexBuffer IndexBufferAddress;
	VertexBuffer VertexBufferAddress;

	uint MaterialBufferIndex;
	uint DrawBatchID;

	int LastVisibilityIndex;
	int CurrentVisibilityIndex;

	float pad0[2];
};

layout(std430, set = 0, binding = 3) readonly buffer DeclMetadataBuffer {

    SceneObjectMetadata ObjectsMetadata[];
} MetadataBuffer;