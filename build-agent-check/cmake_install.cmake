# Install script for directory: /home/krynn/code

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
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/circle" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/circle")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/circle"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/circle")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/circle")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/circle" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/circle")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/circle")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/circle.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/autowiz" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/autowiz")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/autowiz"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/autowiz")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/autowiz")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/autowiz" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/autowiz")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/autowiz")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/autowiz.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/asciipasswd")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/asciipasswd")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/asciipasswd")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/asciipasswd.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/plrtoascii")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/plrtoascii")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/plrtoascii")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/plrtoascii.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/rebuildAsciiIndex")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/rebuildAsciiIndex")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/rebuildAsciiIndex")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/rebuildAsciiIndex.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/rebuildMailIndex")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/rebuildMailIndex")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/rebuildMailIndex")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/rebuildMailIndex.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/shopconv" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/shopconv")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/shopconv"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/shopconv")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/shopconv")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/shopconv" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/shopconv")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/shopconv")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/shopconv.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/sign" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/sign")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/sign"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/sign")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/sign")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/sign" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/sign")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/sign")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/sign.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/split" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/split")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/split"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/split")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/split")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/split" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/split")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/split")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/split.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/wld2html" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/wld2html")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/wld2html"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/wld2html")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/wld2html")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/wld2html" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/wld2html")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/wld2html")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/wld2html.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/webster" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/webster")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/krynn/code/bin/webster"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/krynn/code/bin/webster")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/krynn/code/bin" TYPE EXECUTABLE FILES "/home/krynn/code/bin/webster")
  if(EXISTS "$ENV{DESTDIR}/home/krynn/code/bin/webster" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/krynn/code/bin/webster")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/krynn/code/bin/webster")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/krynn/code/build-agent-check/CMakeFiles/webster.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/krynn/code/build-agent-check/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/krynn/code/build-agent-check/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
