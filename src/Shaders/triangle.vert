#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    mat2 transform;
    vec3 color;
    int useBarycentricColors;
} pc;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 barycentricCoords;

void main() {
    vec2 transformedPos = pc.transform * inPosition;
    gl_Position = vec4(transformedPos, 0.0, 1.0);
    
    // Set barycentric coordinates based on vertex index
    if (gl_VertexIndex == 0) {
        barycentricCoords = vec3(1.0, 0.0, 0.0);
    } else if (gl_VertexIndex == 1) {
        barycentricCoords = vec3(0.0, 1.0, 0.0);
    } else {
        barycentricCoords = vec3(0.0, 0.0, 1.0);
    }
    
    fragColor = pc.color * inColor;
}