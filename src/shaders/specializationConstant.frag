#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;

layout(location = 0) out vec4 outColor;

// Selected at pipeline creation time
layout (constant_id = 0) const int LIGHTING_MODEL = 0;
// Parameter for the toon shading part of the shader
layout (constant_id = 1) const float PARAM_TOON_DESATURATION = 0.0f;

void main() {
//    switch (LIGHTING_MODEL) {
//        case 0: // Phong
//        {
//            vec3 ambient = fragColor * vec3(0.25f);
//            vec3 N = normalize(fragNormal);
//            vec3 L = normalize(fragLightVec);
//            vec3 V = normalize(fragViewVec);
//            vec3 R = reflect(-L, N);
//
//            vec3 diffuse = max(dot(N, L), 0.0) * fragColor;
//            vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.75);
//
//            outColor = vec4(ambient + diffuse * 1.75 + specular, 1.0);
//            break;
//        }
//
//        case 1: // Toon
//        {
//            vec3 ambient = fragColor * vec3(0.25f);
//            vec3 N = normalize(fragNormal);
//            vec3 L = normalize(fragLightVec);
//            vec3 V = normalize(fragViewVec);
//            vec3 R = reflect(-L, N);
//
//            vec3 diffuse = max(dot(N, L), 0.0) * fragColor;
//            vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.75);
//
//            outColor = vec4(ambient + diffuse * 1.75 + specular, 1.0);
//            break;
//        }
//
//        case 3: // Textured
//        {
//            vec4 color = texture(texSampler, fragUV).rrra;
//            vec3 ambient = color.rgb * vec3(0.25) * fragColor;
//            vec3 N = normalize(fragNormal);
//            vec3 L = normalize(fragLightVec);
//            vec3 V = normalize(fragViewVec);
//            vec3 R = reflect(-L, N);
//
//            vec3 diffuse = max(dot(N, L), 0.0) * color.rgb;
//            float specular = pow(max(dot(R, V), 0.0), 32.0) * color.a;
//
//            outColor = vec4(ambient + diffuse + vec3(specular), 1.0);
//            break;
//        }

            vec4 color = texture(texSampler, fragUV).rgba;
            vec3 ambient = color.rgb * vec3(0.25) * fragColor;
            vec3 N = normalize(fragNormal);
            vec3 L = normalize(fragLightVec);
            vec3 V = normalize(fragViewVec);
            vec3 R = reflect(-L, N);

            vec3 diffuse = max(dot(N, L), 0.0) * color.rgb;
            float specular = pow(max(dot(R, V), 0.0), 32.0) * color.a;

            outColor = vec4(ambient + diffuse + vec3(specular), 1.0);
}
