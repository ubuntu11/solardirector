# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /usr/src/sd/utils/sdjs/x/wineditline

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /usr/src/sd/utils/sdjs/x/wineditline

# Include any dependencies generated for this target.
include src/CMakeFiles/edit.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/edit.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/edit.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/edit.dir/flags.make

src/CMakeFiles/edit.dir/editline.c.o: src/CMakeFiles/edit.dir/flags.make
src/CMakeFiles/edit.dir/editline.c.o: src/editline.c
src/CMakeFiles/edit.dir/editline.c.o: src/CMakeFiles/edit.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/usr/src/sd/utils/sdjs/x/wineditline/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/CMakeFiles/edit.dir/editline.c.o"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/edit.dir/editline.c.o -MF CMakeFiles/edit.dir/editline.c.o.d -o CMakeFiles/edit.dir/editline.c.o -c /usr/src/sd/utils/sdjs/x/wineditline/src/editline.c

src/CMakeFiles/edit.dir/editline.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/edit.dir/editline.c.i"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /usr/src/sd/utils/sdjs/x/wineditline/src/editline.c > CMakeFiles/edit.dir/editline.c.i

src/CMakeFiles/edit.dir/editline.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/edit.dir/editline.c.s"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /usr/src/sd/utils/sdjs/x/wineditline/src/editline.c -o CMakeFiles/edit.dir/editline.c.s

src/CMakeFiles/edit.dir/fn_complete.c.o: src/CMakeFiles/edit.dir/flags.make
src/CMakeFiles/edit.dir/fn_complete.c.o: src/fn_complete.c
src/CMakeFiles/edit.dir/fn_complete.c.o: src/CMakeFiles/edit.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/usr/src/sd/utils/sdjs/x/wineditline/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/CMakeFiles/edit.dir/fn_complete.c.o"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/edit.dir/fn_complete.c.o -MF CMakeFiles/edit.dir/fn_complete.c.o.d -o CMakeFiles/edit.dir/fn_complete.c.o -c /usr/src/sd/utils/sdjs/x/wineditline/src/fn_complete.c

src/CMakeFiles/edit.dir/fn_complete.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/edit.dir/fn_complete.c.i"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /usr/src/sd/utils/sdjs/x/wineditline/src/fn_complete.c > CMakeFiles/edit.dir/fn_complete.c.i

src/CMakeFiles/edit.dir/fn_complete.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/edit.dir/fn_complete.c.s"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /usr/src/sd/utils/sdjs/x/wineditline/src/fn_complete.c -o CMakeFiles/edit.dir/fn_complete.c.s

src/CMakeFiles/edit.dir/history.c.o: src/CMakeFiles/edit.dir/flags.make
src/CMakeFiles/edit.dir/history.c.o: src/history.c
src/CMakeFiles/edit.dir/history.c.o: src/CMakeFiles/edit.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/usr/src/sd/utils/sdjs/x/wineditline/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/CMakeFiles/edit.dir/history.c.o"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/edit.dir/history.c.o -MF CMakeFiles/edit.dir/history.c.o.d -o CMakeFiles/edit.dir/history.c.o -c /usr/src/sd/utils/sdjs/x/wineditline/src/history.c

src/CMakeFiles/edit.dir/history.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/edit.dir/history.c.i"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /usr/src/sd/utils/sdjs/x/wineditline/src/history.c > CMakeFiles/edit.dir/history.c.i

src/CMakeFiles/edit.dir/history.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/edit.dir/history.c.s"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /usr/src/sd/utils/sdjs/x/wineditline/src/history.c -o CMakeFiles/edit.dir/history.c.s

# Object files for target edit
edit_OBJECTS = \
"CMakeFiles/edit.dir/editline.c.o" \
"CMakeFiles/edit.dir/fn_complete.c.o" \
"CMakeFiles/edit.dir/history.c.o"

# External object files for target edit
edit_EXTERNAL_OBJECTS =

src/edit.so: src/CMakeFiles/edit.dir/editline.c.o
src/edit.so: src/CMakeFiles/edit.dir/fn_complete.c.o
src/edit.so: src/CMakeFiles/edit.dir/history.c.o
src/edit.so: src/CMakeFiles/edit.dir/build.make
src/edit.so: src/libedit.def
src/edit.so: src/CMakeFiles/edit.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/usr/src/sd/utils/sdjs/x/wineditline/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C shared library edit.so"
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/edit.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/edit.dir/build: src/edit.so
.PHONY : src/CMakeFiles/edit.dir/build

src/CMakeFiles/edit.dir/clean:
	cd /usr/src/sd/utils/sdjs/x/wineditline/src && $(CMAKE_COMMAND) -P CMakeFiles/edit.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/edit.dir/clean

src/CMakeFiles/edit.dir/depend:
	cd /usr/src/sd/utils/sdjs/x/wineditline && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /usr/src/sd/utils/sdjs/x/wineditline /usr/src/sd/utils/sdjs/x/wineditline/src /usr/src/sd/utils/sdjs/x/wineditline /usr/src/sd/utils/sdjs/x/wineditline/src /usr/src/sd/utils/sdjs/x/wineditline/src/CMakeFiles/edit.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/edit.dir/depend
