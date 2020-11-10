#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 2) uniform UniformBufferObjectLight {
    vec4 lightPos;
} uboLight;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragViewVec;
layout(location = 4) out vec3 fragLightVec;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    fragColor = inColor;
    fragUV = inUV;
    fragNormal = inNormal;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    vec4 pos = ubo.model * vec4(inPosition, 1.0);
    fragNormal = mat3(ubo.model) * inNormal;

    fragViewVec = -pos.xyz;

    vec3 lPos = mat3(ubo.model) * uboLight.lightPos.xyz;
    fragLightVec = lPos - pos.xyz;
}
