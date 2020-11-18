CXX ?= g++

# path #
SRC_PATH = src
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin

# Space-separated pkg-config libraries used by this project
BASE_PATH=include/VulkanLearning/base
VK_LEARNING_PATH=include/VulkanLearning
GLM_PATH=external/glm
STB_PATH=include/external/stb
TINY_OBJ_LOADER_PATH=include/external/tinyobjloader
IMGUI_PATH=src/external/imgui
KTX_PATH=external/KTX-Software/include
KTX_OTHER_PATH=external/KTX-Software/other_include

INCLUDES_EXT=-I$(BASE_PATH) -I$(VK_LEARNING_PATH) -I$(STB_PATH) -I$(TINY_OBJ_LOADER_PATH) -I$(IMGUI_PATH) -I$(KTX_PATH) -I$(GLM_PATH)

# extensions #
SRC_EXT = cpp

SOURCES=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
OBJECTS=$(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)

LIBS = `pkg-config --static --libs glfw3` -lvulkan -lglfw3
CFLAGS = -std=c++17 -g $(INCLUDES_EXT) `pkg-config --cflags glfw3`

EXAMPLES = single3DModel simpleTriangle dynamicUniformBuffers pushConstants specializationConstant texture

.PHONY: default_target
default_target: all

.PHONY: all
all: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS) $(CFLAGS)
all: dirs
	@$(MAKE) $(EXAMPLES)

#####################################################
################Simple3DModel
#####################################################

SOURCES_1=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f2-) src/examples/single3DModel/single3DModel.cpp
OBJECTS_1=$(SOURCES_1:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_1= $(OBJECTS_1:.o=.d)
BIN_NAME_1=single3DModel
.PHONY: single3DModel
single3DModel: $(BIN_PATH)/$(BIN_NAME_1)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_1) $(BIN_NAME_1)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_1): $(OBJECTS_1)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_1) -o $@ ${LIBS}

#####################################################
###############SimpleTriangle
#####################################################

SOURCES_2=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f2-) src/examples/simpleTriangle/simpleTriangle.cpp
OBJECTS_2=$(SOURCES_2:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_2= $(OBJECTS_2:.o=.d)
BIN_NAME_2=simpleTriangle
.PHONY: simpleTriangle
simpleTriangle: $(BIN_PATH)/$(BIN_NAME_2)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_2) $(BIN_NAME_2)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_2): $(OBJECTS_2)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_2) -o $@ ${LIBS}

#####################################################
##############Dynamic Uniform Buffers
#####################################################

SOURCES_3=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f3-) src/examples/dynamicUniformBuffers/dynamicUniformBuffers.cpp
OBJECTS_3=$(SOURCES_3:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_3= $(OBJECTS_3:.o=.d)
BIN_NAME_3=dynamicUniformBuffers
.PHONY: dynamicUniformBuffers
dynamicUniformBuffers: $(BIN_PATH)/$(BIN_NAME_3)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_3) $(BIN_NAME_3)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_3): $(OBJECTS_3)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_3) -o $@ ${LIBS}

#####################################################
#####################################################
#####################################################

#####################################################
############## Push Constants
#####################################################

SOURCES_4=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f3-) src/examples/pushConstants/pushConstants.cpp
OBJECTS_4=$(SOURCES_4:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_4= $(OBJECTS_4:.o=.d)
BIN_NAME_4=pushConstants
.PHONY: pushConstants
pushConstants: $(BIN_PATH)/$(BIN_NAME_4)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_4) $(BIN_NAME_4)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_4): $(OBJECTS_4)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_4) -o $@ ${LIBS}

#####################################################
#####################################################
#####################################################

#####################################################
############## SpecializationConstant
#####################################################

SOURCES_5=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f3-) src/examples/specializationConstant/specializationConstant.cpp
OBJECTS_5=$(SOURCES_5:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_5= $(OBJECTS_5:.o=.d)
BIN_NAME_5=specializationConstant
.PHONY: specializationConstant
specializationConstant: $(BIN_PATH)/$(BIN_NAME_5)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_5) $(BIN_NAME_5)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_5): $(OBJECTS_5)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_5) -o $@ ${LIBS}

#####################################################
#####################################################
#####################################################

#####################################################
################ Texture
#####################################################

SOURCES_6=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' ! -path "src/examples/*" | sort -k 1nr | cut -f2-) src/examples/texture/texture.cpp
OBJECTS_6=$(SOURCES_6:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPX_6= $(OBJECTS_6:.o=.d)
BIN_NAME_6=texture
.PHONY: texture
texture: $(BIN_PATH)/$(BIN_NAME_6)
	@echo "Making symlink: $@ -> $<"
	@$(RM) $
	@ln -sfn $(BIN_PATH)/$(BIN_NAME_6) $(BIN_NAME_6)

# Creation of the executable
$(BIN_PATH)/$(BIN_NAME_6): $(OBJECTS_6)
	@echo "Linking: $@"
	$(CXX) $(OBJECTS_6) -o $@ ${LIBS}

#####################################################
#####################################################
#####################################################

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CXXFLAGS) -MP -MMD -c $< -o $@


.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS_1))
	@mkdir -p $(dir $(OBJECTS_2))
	@mkdir -p $(dir $(OBJECTS_3))
	@mkdir -p $(dir $(OBJECTS_4))
	@mkdir -p $(dir $(OBJECTS_5))
	@mkdir -p $(dir $(OBJECTS_6))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@echo "Cleaning"
	@$(RM) $(OBJECTS)
	@$(RM) $(EXAMPLES)
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)
