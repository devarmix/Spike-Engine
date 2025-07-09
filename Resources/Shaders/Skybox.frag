#version 450

layout(location = 0) in vec3 viewDir;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 5) uniform samplerCube skyboxMap;

void main() {

    vec3 color = texture(skyboxMap, normalize(viewDir)).rgb;
    outColor = vec4(color, 1.0f);
}