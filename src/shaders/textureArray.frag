#version 450

layout (binding = 1) uniform sampler2DArray samplerColorArray;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
    outFragColor = texture(samplerColorArray, inUV); 
    //outFragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
