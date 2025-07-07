#version 450

layout(location = 0) out vec2 fragUV;

void main() {
   
    vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0), // Bottom-left
        vec2( 1.0, -1.0), // Bottom-right
        vec2(-1.0,  1.0), // Top-left
        vec2(-1.0,  1.0), // Top-left
        vec2( 1.0, -1.0), // Bottom-right
        vec2( 1.0,  1.0)  // Top-right
    );

    vec2 pos = positions[gl_VertexIndex];
    fragUV = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}