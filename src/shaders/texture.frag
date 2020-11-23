#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = texture(samplerColor, inUV, 0.0f);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = reflect(L, N);

    vec3 diffuse = max(dot(N, L), 0.0f) * vec3(1.0f);
    float specular = pow(max(dot(R, V), 0.0f), 16.0f) * color.a;

    outFragColor = vec4(diffuse * color.rgb + specular, 1.0f);
}
