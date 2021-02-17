#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
    vec4 lightPos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outPos;

void main() 
{
	outUV = inUV;
	outPos = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
	gl_Position = outPos;		
}