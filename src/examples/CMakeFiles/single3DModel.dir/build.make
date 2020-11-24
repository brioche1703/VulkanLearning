# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/brioche/dev/graphics/vulkan/VulkanLearning

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/brioche/dev/graphics/vulkan/VulkanLearning

# Include any dependencies generated for this target.
include src/examples/CMakeFiles/single3DModel.dir/depend.make

# Include the progress variables for this target.
include src/examples/CMakeFiles/single3DModel.dir/progress.make

# Include the compile flags for this target's objects.
include src/examples/CMakeFiles/single3DModel.dir/flags.make

src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o: src/examples/CMakeFiles/single3DModel.dir/flags.make
src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o: src/examples/single3DModel/single3DModel.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/brioche/dev/graphics/vulkan/VulkanLearning/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o -c /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/single3DModel/single3DModel.cpp

src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.i"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/single3DModel/single3DModel.cpp > CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.i

src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.s"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/single3DModel/single3DModel.cpp -o CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.s

# Object files for target single3DModel
single3DModel_OBJECTS = \
"CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o"

# External object files for target single3DModel
single3DModel_EXTERNAL_OBJECTS =

bin/single3DModel: src/examples/CMakeFiles/single3DModel.dir/single3DModel/single3DModel.cpp.o
bin/single3DModel: src/examples/CMakeFiles/single3DModel.dir/build.make
bin/single3DModel: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/single3DModel: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/single3DModel: src/base/libbase.a
bin/single3DModel: external/glfw-3.3.2/src/libglfw3.a
bin/single3DModel: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/single3DModel: /usr/lib/x86_64-linux-gnu/librt.so
bin/single3DModel: /usr/lib/x86_64-linux-gnu/libm.so
bin/single3DModel: /usr/lib/x86_64-linux-gnu/libX11.so
bin/single3DModel: src/examples/CMakeFiles/single3DModel.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/brioche/dev/graphics/vulkan/VulkanLearning/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/single3DModel"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/single3DModel.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/examples/CMakeFiles/single3DModel.dir/build: bin/single3DModel

.PHONY : src/examples/CMakeFiles/single3DModel.dir/build

src/examples/CMakeFiles/single3DModel.dir/clean:
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && $(CMAKE_COMMAND) -P CMakeFiles/single3DModel.dir/cmake_clean.cmake
.PHONY : src/examples/CMakeFiles/single3DModel.dir/clean

src/examples/CMakeFiles/single3DModel.dir/depend:
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/brioche/dev/graphics/vulkan/VulkanLearning /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples /home/brioche/dev/graphics/vulkan/VulkanLearning /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/CMakeFiles/single3DModel.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/examples/CMakeFiles/single3DModel.dir/depend
