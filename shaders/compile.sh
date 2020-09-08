if [[ "$OSTYPE" == 'linux' ]]; then
    GLSLC_PATH="HOME/dev/graphics/lib/VulkanSDK/1.2.148.1/x86_64/bin/glslc"
else
    GLSLC_PATH="$HOME/dev/lib/vulkan_macos_1.2.148.1/macOS/bin/glslc"
fi

echo $GLSLC_PATH
$GLSLC_PATH shader.vert -o vert.spv
$GLSLC_PATH shader.frag -o frag.spv
