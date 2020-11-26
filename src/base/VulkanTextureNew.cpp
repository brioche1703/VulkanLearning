#include "VulkanTextureNew.hpp"


namespace VulkanLearning {
    
    ktxResult VulkanTextureNew::loadKTXFile(std::string filename, ktxTexture **target) {
        ktxResult result = KTX_SUCCESS;

        result = ktxTexture_CreateFromNamedFile(
                filename.c_str(), 
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, 
                target);

        if (result != KTX_SUCCESS) {
            throw std::runtime_error("KTX Texture :" + filename + " creation failed!");
        }

        return result;
    }

}
