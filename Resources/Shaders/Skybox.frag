#version 450

layout(location = 0) in vec3 viewDir;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube skyboxMap;


void main() {

    outColor = texture(skyboxMap, normalize(viewDir));
}