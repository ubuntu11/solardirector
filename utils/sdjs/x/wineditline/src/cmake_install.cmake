# Install script for directory: /usr/src/sd/utils/sdjs/x/wineditline/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/src/sd/utils/sdjs/x/wineditline/bin64" TYPE SHARED_LIBRARY FILES "/usr/src/sd/utils/sdjs/x/wineditline/src/edit.so")
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/src/sd/utils/sdjs/x/wineditline/bin64" TYPE EXECUTABLE FILES "/usr/src/sd/utils/sdjs/x/wineditline/src/edit_test")
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test"
         OLD_RPATH "/usr/src/sd/utils/sdjs/x/wineditline/src:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/src/sd/utils/sdjs/x/wineditline/bin64" TYPE EXECUTABLE FILES "/usr/src/sd/utils/sdjs/x/wineditline/src/edit_test_dll")
  if(EXISTS "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/src/sd/utils/sdjs/x/wineditline/bin64/edit_test_dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/src/sd/utils/sdjs/x/wineditline/lib64/libedit_static.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/src/sd/utils/sdjs/x/wineditline/lib64" TYPE STATIC_LIBRARY FILES "/usr/src/sd/utils/sdjs/x/wineditline/src/libedit_static.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/src/sd/utils/sdjs/x/wineditline/include/editline/readline.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/src/sd/utils/sdjs/x/wineditline/include/editline" TYPE FILE FILES "/usr/src/sd/utils/sdjs/x/wineditline/src/editline/readline.h")
endif()

