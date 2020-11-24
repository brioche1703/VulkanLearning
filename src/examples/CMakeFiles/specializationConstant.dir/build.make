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
include src/examples/CMakeFiles/specializationConstant.dir/depend.make

# Include the progress variables for this target.
include src/examples/CMakeFiles/specializationConstant.dir/progress.make

# Include the compile flags for this target's objects.
include src/examples/CMakeFiles/specializationConstant.dir/flags.make

src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o: src/examples/CMakeFiles/specializationConstant.dir/flags.make
src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o: src/examples/specializationConstant/specializationConstant.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/brioche/dev/graphics/vulkan/VulkanLearning/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o -c /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/specializationConstant/specializationConstant.cpp

src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.i"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/specializationConstant/specializationConstant.cpp > CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.i

src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.s"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/specializationConstant/specializationConstant.cpp -o CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.s

# Object files for target specializationConstant
specializationConstant_OBJECTS = \
"CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o"

# External object files for target specializationConstant
specializationConstant_EXTERNAL_OBJECTS =

bin/specializationConstant: src/examples/CMakeFiles/specializationConstant.dir/specializationConstant/specializationConstant.cpp.o
bin/specializationConstant: src/examples/CMakeFiles/specializationConstant.dir/build.make
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/specializationConstant: src/base/libbase.a
bin/specializationConstant: external/glfw-3.3.2/src/libglfw3.a
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/libvulkan.so
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/librt.so
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/libm.so
bin/specializationConstant: /usr/lib/x86_64-linux-gnu/libX11.so
bin/specializationConstant: src/examples/CMakeFiles/specializationConstant.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/brioche/dev/graphics/vulkan/VulkanLearning/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/specializationConstant"
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/specializationConstant.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/examples/CMakeFiles/specializationConstant.dir/build: bin/specializationConstant

.PHONY : src/examples/CMakeFiles/specializationConstant.dir/build

src/examples/CMakeFiles/specializationConstant.dir/clean:
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples && $(CMAKE_COMMAND) -P CMakeFiles/specializationConstant.dir/cmake_clean.cmake
.PHONY : src/examples/CMakeFiles/specializationConstant.dir/clean

src/examples/CMakeFiles/specializationConstant.dir/depend:
	cd /home/brioche/dev/graphics/vulkan/VulkanLearning && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/brioche/dev/graphics/vulkan/VulkanLearning /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples /home/brioche/dev/graphics/vulkan/VulkanLearning /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples /home/brioche/dev/graphics/vulkan/VulkanLearning/src/examples/CMakeFiles/specializationConstant.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/examples/CMakeFiles/specializationConstant.dir/depend
