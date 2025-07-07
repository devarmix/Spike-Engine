#version 450

layout(push_constant) uniform PushData {

    mat4 MVP;
    float Roughness;

    float pad0[3];

} Data;

layout(location = 0) out vec3 outUVW;

vec3 Positions[36] = vec3[](
    
    vec3(-1, -1, -1), vec3( 1, -1, -1), vec3( 1,  1, -1),
    vec3(-1, -1, -1), vec3( 1,  1, -1), vec3(-1,  1, -1),

    vec3(-1, -1,  1), vec3( 1, -1,  1), vec3( 1,  1,  1),
    vec3(-1, -1,  1), vec3( 1,  1,  1), vec3(-1,  1,  1),

    vec3(-1, -1, -1), vec3(-1,  1, -1), vec3(-1,  1,  1),
    vec3(-1, -1, -1), vec3(-1,  1,  1), vec3(-1, -1,  1),

    vec3( 1, -1, -1), vec3( 1,  1, -1), vec3( 1,  1,  1),
    vec3( 1, -1, -1), vec3( 1,  1,  1), vec3( 1, -1,  1),

    vec3(-1, -1, -1), vec3(-1, -1,  1), vec3( 1, -1,  1),
    vec3(-1, -1, -1), vec3( 1, -1,  1), vec3( 1, -1, -1),

    vec3(-1,  1, -1), vec3(-1,  1,  1), vec3( 1,  1,  1),
    vec3(-1,  1, -1), vec3( 1,  1,  1), vec3( 1,  1, -1)
);

void main() {

    vec3 inPos = Positions[gl_VertexIndex];
    outUVW = inPos;
    gl_Position = Data.MVP * vec4(inPos, 1.0);
}