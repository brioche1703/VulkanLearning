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
