#version 450

layout(location = 0) out vec2 fragUV;

void main() {
    // Generate a full-screen quad using vertex ID
    vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0), // Bottom-left
        vec2( 1.0, -1.0), // Bottom-right
        vec2(-1.0,  1.0), // Top-left
        vec2(-1.0,  1.0), // Top-left
        vec2( 1.0, -1.0), // Bottom-right
        vec2( 1.0,  1.0)  // Top-right
    );

    // Get the position for the current vertex
    vec2 pos = positions[gl_VertexIndex];

    // Set the UV coordinates (mapping from [-1, 1] to [0, 1])
    fragUV = pos * 0.5 + 0.5;

    // Output the position directly as the clip-space position
    gl_Position = vec4(pos, 0.0, 1.0);
}