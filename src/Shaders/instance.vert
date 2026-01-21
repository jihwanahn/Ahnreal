#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    // ... light info etc
} camera;

// Same binding slot as Compute Shader for Instance Data? 
// No, Graphics Pipeline will have its own descriptor set layout.
// Let's assume Set 1 Binding 0 is Instance Data, Set 1 Binding 1 is Visible Indices (SSBO)

layout(std140, set = 1, binding = 0) readonly buffer InstanceData {
    mat4 modelMatrix[];
} instances;

layout(std140, set = 1, binding = 1) readonly buffer VisibleInstances {
    uint indices[];
} visibleInstances;

void main() {
    // Indirect Draw: gl_InstanceIndex counts from 0 to instanceCount-1 (the visible ones)
    // We need to look up the ACTUAL original instance index
    uint originalIndex = visibleInstances.indices[gl_InstanceIndex];
    
    mat4 model = instances.modelMatrix[originalIndex];
    mat4 mvp = camera.proj * camera.view * model;
    
    gl_Position = mvp * vec4(inPosition, 1.0);
    
    // Simple color based on normal
    fragColor = (inNormal + 1.0) * 0.5; 
    fragTexCoord = inTexCoord;
}
