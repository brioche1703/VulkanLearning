OS="`uname`"

case $OS in
    'Linux')
        #GLSLC_PATH="$HOME/dev/graphics/lib/VulkanSDK/1.2.148.1/x86_64/bin/glslc"
        GLSLC_PATH="/home/brioche/dev/lib/vulkan_1.2.162.0/x86_64/bin/glslc"
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

$GLSLC_PATH textureArray.vert -o textureArrayVert.spv
$GLSLC_PATH textureArray.frag -o textureArrayFrag.spv

$GLSLC_PATH glTFLoading.vert -o glTFLoadingVert.spv
$GLSLC_PATH glTFLoading.frag -o glTFLoadingFrag.spv

$GLSLC_PATH glTFScene.vert -o glTFSceneVert.spv
$GLSLC_PATH glTFScene.frag -o glTFSceneFrag.spv

$GLSLC_PATH glTFCompleteLoader.vert -o glTFCompleteLoaderVert.spv
$GLSLC_PATH glTFCompleteLoader.frag -o glTFCompleteLoaderFrag.spv

$GLSLC_PATH ui.vert -o uiVert.spv
$GLSLC_PATH ui.frag -o uiFrag.spv

$GLSLC_PATH textureCubemap/reflect.vert -o textureCubemap/reflectVert.spv
$GLSLC_PATH textureCubemap/reflect.frag -o textureCubemap/reflectFrag.spv

$GLSLC_PATH textureCubemap/skybox.vert -o textureCubemap/skyboxVert.spv
$GLSLC_PATH textureCubemap/skybox.frag -o textureCubemap/skyboxFrag.spv

$GLSLC_PATH textureCubemapArray/reflect.vert -o textureCubemapArray/reflectVert.spv
$GLSLC_PATH textureCubemapArray/reflect.frag -o textureCubemapArray/reflectFrag.spv

$GLSLC_PATH textureCubemapArray/skybox.vert -o textureCubemapArray/skyboxVert.spv
$GLSLC_PATH textureCubemapArray/skybox.frag -o textureCubemapArray/skyboxFrag.spv

$GLSLC_PATH texture3d/texture3d.vert -o texture3d/texture3dVert.spv
$GLSLC_PATH texture3d/texture3d.frag -o texture3d/texture3dFrag.spv

$GLSLC_PATH inputAttachments/inputAttachments.vert -o inputAttachments/inputAttachmentsVert.spv
$GLSLC_PATH inputAttachments/inputAttachments.frag -o inputAttachments/inputAttachmentsFrag.spv
