UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
	VULKAN_SDK_PATH = $(HOME)/dev/graphics/lib/VulkanSDK/1.2.148.1/x86_64
else
	VULKAN_SDK_PATH = $(HOME)/dev/lib/vulkan_macos_1.2.148.1/macOS
endif

STB_PATH = ./libs/stb

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_PATH) `pkg-config --cflags glfw3`
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan -lglfw3
VulkanTest: main.cpp
	g++ $(CFLAGS) -o a.out main.cpp $(LDFLAGS)

.PHONY: test clean

test: a.out
	LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib;$(STB_PATH) VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/vulkan/explicit_layer.d ./a.out

clean:
	rm -f a.out
