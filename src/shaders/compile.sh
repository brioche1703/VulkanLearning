OS="`uname`"

case $OS in
    'Linux')
        GLSLC_PATH="$HOME/dev/graphics/lib/VulkanSDK/1.2.148.1/x86_64/bin/glslc"
        ;;
    'Darwin')
        GLSLC_PATH="$HOME/dev/lib/vulkan_macos_1.2.148.1/macOS/bin/glslc"
        ;;
esac

$GLSLC_PATH shader.vert -o vert.spv
$GLSLC_PATH shader.frag -o frag.spv

$GLSLC_PATH single3DModel.vert -o single3DModelVert.spv
$GLSLC_PATH single3DModel.frag -o single3DModelFrag.spv

$GLSLC_PATH simpleTriangleShader.vert -o simpleTriangleShaderVert.spv
$GLSLC_PATH simpleTriangleShader.frag -o simpleTriangleShaderFrag.spv

$GLSLC_PATH dynamicUniformBuffers.vert -o dynamicUniformBuffersVert.spv
$GLSLC_PATH dynamicUniformBuffers.frag -o dynamicUniformBuffersFrag.spv

$GLSLC_PATH pushConstants.vert -o pushConstantsVert.spv
$GLSLC_PATH pushConstants.frag -o pushConstantsFrag.spv

$GLSLC_PATH specializationConstant.vert -o specializationConstantVert.spv
$GLSLC_PATH specializationConstant.frag -o specializationConstantFrag.spv

$GLSLC_PATH texture.vert -o textureVert.spv
$GLSLC_PATH texture.frag -o textureFrag.spv
