# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

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
CMAKE_COMMAND = "/Applications/CMake 2.8-10.app/Contents/bin/cmake"

# The command to remove a file.
RM = "/Applications/CMake 2.8-10.app/Contents/bin/cmake" -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = "/Applications/CMake 2.8-10.app/Contents/bin/ccmake"

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/phil/Documents/development/hg/openexr-cmake

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/phil/Documents/development/hg/openexr-cmake

# Include any dependencies generated for this target.
include OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/depend.make

# Include the progress variables for this target.
include OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/progress.make

# Include the compile flags for this target's objects.
include OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/flags.make

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/flags.make
OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o: OpenEXR/exr2aces/main.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /Users/phil/Documents/development/hg/openexr-cmake/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o"
	cd /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/exr2aces.dir/main.cpp.o -c /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces/main.cpp

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/exr2aces.dir/main.cpp.i"
	cd /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces/main.cpp > CMakeFiles/exr2aces.dir/main.cpp.i

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/exr2aces.dir/main.cpp.s"
	cd /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces/main.cpp -o CMakeFiles/exr2aces.dir/main.cpp.s

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.requires:
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.requires

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.provides: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.requires
	$(MAKE) -f OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/build.make OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.provides.build
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.provides

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.provides.build: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o

# Object files for target exr2aces
exr2aces_OBJECTS = \
"CMakeFiles/exr2aces.dir/main.cpp.o"

# External object files for target exr2aces
exr2aces_EXTERNAL_OBJECTS =

OpenEXR/exr2aces/exr2aces: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o
OpenEXR/exr2aces/exr2aces: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/build.make
OpenEXR/exr2aces/exr2aces: OpenEXR/IlmImf/libIlmImf.1.7.1.dylib
OpenEXR/exr2aces/exr2aces: /usr/lib/libz.dylib
OpenEXR/exr2aces/exr2aces: IlmBase/Half/libHalf.1.0.3.dylib
OpenEXR/exr2aces/exr2aces: IlmBase/IlmThread/libIlmThread.1.0.3.dylib
OpenEXR/exr2aces/exr2aces: IlmBase/Iex/libIex.1.0.3.dylib
OpenEXR/exr2aces/exr2aces: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable exr2aces"
	cd /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/exr2aces.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/build: OpenEXR/exr2aces/exr2aces
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/build

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/requires: OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/main.cpp.o.requires
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/requires

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/clean:
	cd /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces && $(CMAKE_COMMAND) -P CMakeFiles/exr2aces.dir/cmake_clean.cmake
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/clean

OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/depend:
	cd /Users/phil/Documents/development/hg/openexr-cmake && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/phil/Documents/development/hg/openexr-cmake /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces /Users/phil/Documents/development/hg/openexr-cmake /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces /Users/phil/Documents/development/hg/openexr-cmake/OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : OpenEXR/exr2aces/CMakeFiles/exr2aces.dir/depend

