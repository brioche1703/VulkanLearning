#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout (binding = 2) uniform UBO {
    vec2 brightnessConstrast;
    vec2 range;
    int attachmentIndex;
} ubo;

layout (location = 0) out vec4 outColor;

vec3 brightnessConstrast(vec3 color, float brightness, float contrast) {
    return (color - 0.5) * contrast + 0.5 + brightness;
}

void main() 
{
    if (ubo.attachmentIndex == 0) {
        vec3 color = subpassLoad(inputColor).rgb;
        outColor.rgb = brightnessConstrast(color, ubo.brightnessConstrast[0], ubo.brightnessConstrast[1]);
    }

    if (ubo.attachmentIndex == 1) {
        float depth = subpassLoad(inputDepth).r;
        outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
    }
}
