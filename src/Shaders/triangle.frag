#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 barycentricCoords;

layout(push_constant) uniform PushConstants {
    mat2 transform;
    vec3 color;
    int useBarycentricColors;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    if (pc.useBarycentricColors == 1) {
        outColor = vec4(barycentricCoords, 1.0);
    } else {
        outColor = vec4(fragColor, 1.0);
    }
}