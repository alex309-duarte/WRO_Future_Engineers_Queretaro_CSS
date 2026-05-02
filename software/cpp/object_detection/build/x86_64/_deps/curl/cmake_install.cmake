# Install script for directory: /home/maker/hailo-apps/hailo_apps/cpp/external/curl

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/docs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/scripts/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/docs/examples/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/tests/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/curl-config")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/libcurl.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/include/curl" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL/CURLTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL/CURLTargets.cmake"
         "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/CMakeFiles/Export/8e83d16133499b505bf3986f4f209a65/CURLTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL/CURLTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL/CURLTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL" TYPE FILE FILES "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/CMakeFiles/Export/8e83d16133499b505bf3986f4f209a65/CURLTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL" TYPE FILE FILES "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/CMakeFiles/Export/8e83d16133499b505bf3986f4f209a65/CURLTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/CURL" TYPE FILE FILES
    "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/generated/CURLConfigVersion.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/generated/CURLConfig.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindBrotli.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindCares.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindGSS.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindGnuTLS.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLDAP.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibbacktrace.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibgsasl.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibidn2.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibpsl.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibssh.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibssh2.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindLibuv.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindMbedTLS.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindNGHTTP2.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindNGHTTP3.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindNGTCP2.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindNettle.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindQuiche.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindRustls.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindWolfSSL.cmake"
    "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/CMake/FindZstd.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/home/maker/hailo-apps/hailo_apps/cpp/external/curl/scripts/wcurl")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/maker/hailo-apps/hailo_apps/cpp/object_detection/build/x86_64/_deps/curl/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
