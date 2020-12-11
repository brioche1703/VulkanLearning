#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

struct Instance
{
    mat4 model;
    vec4 arrayIndex;
};

struct CoordinateUBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
    vec3 camPos;
};

layout (binding = 0) uniform UBO 
{
    CoordinateUBO coordUbo;
    Instance instance[7];
} ubo;

layout (location = 0) out vec3 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
    outUV = vec3(inUV, ubo.instance[gl_InstanceIndex].arrayIndex.x);
    mat4 modelView = ubo.coordUbo.view * ubo.instance[gl_InstanceIndex].model;

	gl_Position = ubo.coordUbo.proj * modelView * vec4(inPos.xyz, 1.0);
}
