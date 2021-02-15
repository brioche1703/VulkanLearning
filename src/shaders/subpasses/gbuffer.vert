#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outWorldPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * inPos;
	
	// Vertex position in world space
	outWorldPos = vec3(ubo.model * inPos);
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	outNormal = mNormal * normalize(inNormal);	
	
	outColor = inColor;
}
