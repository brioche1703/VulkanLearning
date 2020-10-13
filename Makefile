CXX ?= g++

# path #
SRC_PATH = src
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin

# Space-separated pkg-config libraries used by this project
STB_PATH = ./include/stb
TINY_OBJ_LOADER_PATH = ./include/tinyobjloader
IMGUI_PATH = ./src/external/imgui
INCLUDES_EXT = -I$(STB_PATH)$(TINY_OBJ_LOADER_PATH)$(IMGUI_PATH)$(MISC_PATH)

# extensions #
SRC_EXT = cpp

SOURCES=$(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
OBJECTS=$(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)

LIBS = `pkg-config --static --libs glfw3` -lvulkan -lglfw3
CFLAGS = -std=c++17 $(INCLUDES_EXT) `pkg-config --cflags glfw3`

EXAMPLES = single3DModel simpleTriangle dynamicUniformBuffers

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


$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

#####################################################
#####################################################
#####################################################

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS_1))
	@mkdir -p $(dir $(OBJECTS_2))
	@mkdir -p $(dir $(OBJECTS_3))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@echo "Cleaning"
	@$(RM) $(OBJECTS)
	@$(RM) $(EXAMPLES)
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)
