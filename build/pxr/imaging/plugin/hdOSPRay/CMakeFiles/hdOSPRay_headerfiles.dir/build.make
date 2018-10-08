# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_SOURCE_DIR = /home/carson/git/hdOSPRayPlugin

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carson/git/hdOSPRayPlugin/build

# Utility rule file for hdOSPRay_headerfiles.

# Include the progress variables for this target.
include pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/progress.make

pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/context.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderParam.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/config.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/instancer.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/mesh.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/rendererPlugin.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderDelegate.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderPass.h
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/sampler.h


include/pxr/imaging/hdOSPRay/context.h: ../pxr/imaging/plugin/hdOSPRay/context.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Copying context.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/context.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/context.h

include/pxr/imaging/hdOSPRay/renderParam.h: ../pxr/imaging/plugin/hdOSPRay/renderParam.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Copying renderParam.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderParam.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/renderParam.h

include/pxr/imaging/hdOSPRay/config.h: ../pxr/imaging/plugin/hdOSPRay/config.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Copying config.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/config.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/config.h

include/pxr/imaging/hdOSPRay/instancer.h: ../pxr/imaging/plugin/hdOSPRay/instancer.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Copying instancer.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/instancer.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/instancer.h

include/pxr/imaging/hdOSPRay/mesh.h: ../pxr/imaging/plugin/hdOSPRay/mesh.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Copying mesh.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/mesh.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/mesh.h

include/pxr/imaging/hdOSPRay/rendererPlugin.h: ../pxr/imaging/plugin/hdOSPRay/rendererPlugin.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Copying rendererPlugin.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/rendererPlugin.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/rendererPlugin.h

include/pxr/imaging/hdOSPRay/renderDelegate.h: ../pxr/imaging/plugin/hdOSPRay/renderDelegate.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Copying renderDelegate.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderDelegate.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/renderDelegate.h

include/pxr/imaging/hdOSPRay/renderPass.h: ../pxr/imaging/plugin/hdOSPRay/renderPass.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Copying renderPass.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderPass.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/renderPass.h

include/pxr/imaging/hdOSPRay/sampler.h: ../pxr/imaging/plugin/hdOSPRay/sampler.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/carson/git/hdOSPRayPlugin/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Copying sampler.h ..."
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && /usr/bin/python2.7 /home/carson/git/hdOSPRayPlugin/cmake/macros/copyHeaderForBuild.py /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/sampler.h /home/carson/git/hdOSPRayPlugin/build//include/pxr/imaging/hdOSPRay/sampler.h

hdOSPRay_headerfiles: pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/context.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderParam.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/config.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/instancer.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/mesh.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/rendererPlugin.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderDelegate.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/renderPass.h
hdOSPRay_headerfiles: include/pxr/imaging/hdOSPRay/sampler.h
hdOSPRay_headerfiles: pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/build.make

.PHONY : hdOSPRay_headerfiles

# Rule to build all files generated by this target.
pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/build: hdOSPRay_headerfiles

.PHONY : pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/build

pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/clean:
	cd /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay && $(CMAKE_COMMAND) -P CMakeFiles/hdOSPRay_headerfiles.dir/cmake_clean.cmake
.PHONY : pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/clean

pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/depend:
	cd /home/carson/git/hdOSPRayPlugin/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carson/git/hdOSPRayPlugin /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay /home/carson/git/hdOSPRayPlugin/build /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay /home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : pxr/imaging/plugin/hdOSPRay/CMakeFiles/hdOSPRay_headerfiles.dir/depend

