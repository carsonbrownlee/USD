# Install script for directory: /home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/carson/git/USD/build/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so"
         RPATH "$ORIGIN/../../lib:/home/carson/git/USD/build/install/lib:/home/carson/opt/embree-3.2.0.x86_64.linux/lib:/home/carson/git/ospray/build/install/lib64")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/plugin/usd" TYPE SHARED_LIBRARY FILES "/home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay/hdOSPRay.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so"
         OLD_RPATH "/home/carson/git/hdOSPRayPlugin/arch:/home/carson/git/hdOSPRayPlugin/tf:/home/carson/git/hdOSPRayPlugin/gf:/home/carson/git/hdOSPRayPlugin/js:/home/carson/git/hdOSPRayPlugin/trace:/home/carson/git/hdOSPRayPlugin/work:/home/carson/git/hdOSPRayPlugin/plug:/home/carson/git/hdOSPRayPlugin/vt:/home/carson/git/hdOSPRayPlugin/ar:/home/carson/git/hdOSPRayPlugin/kind:/home/carson/git/hdOSPRayPlugin/sdf:/home/carson/git/hdOSPRayPlugin/ndr:/home/carson/git/hdOSPRayPlugin/sdr:/home/carson/git/hdOSPRayPlugin/pcp:/home/carson/git/hdOSPRayPlugin/usd:/home/carson/git/hdOSPRayPlugin/usdGeom:/home/carson/git/hdOSPRayPlugin/usdVol:/home/carson/git/hdOSPRayPlugin/usdLux:/home/carson/git/hdOSPRayPlugin/usdShade:/home/carson/git/hdOSPRayPlugin/usdHydra:/home/carson/git/hdOSPRayPlugin/usdRi:/home/carson/git/hdOSPRayPlugin/usdSkel:/home/carson/git/hdOSPRayPlugin/usdUI:/home/carson/git/hdOSPRayPlugin/usdUtils:/home/carson/git/hdOSPRayPlugin/garch:/home/carson/git/hdOSPRayPlugin/hf:/home/carson/git/hdOSPRayPlugin/cameraUtil:/home/carson/git/hdOSPRayPlugin/pxOsd:/home/carson/git/hdOSPRayPlugin/glf:/home/carson/git/hdOSPRayPlugin/hd:/home/carson/git/hdOSPRayPlugin/hdSt:/home/carson/git/hdOSPRayPlugin/hdx:/home/carson/git/hdOSPRayPlugin/hdStream:/home/carson/git/hdOSPRayPlugin/hdEmbree:/home/carson/git/hdOSPRayPlugin/usdImaging:/home/carson/git/hdOSPRayPlugin/usdImagingGL:/home/carson/git/hdOSPRayPlugin/usdShaders:/home/carson/git/hdOSPRayPlugin/usdSkelImaging:/home/carson/git/hdOSPRayPlugin/usdVolImaging:/home/carson/git/hdOSPRayPlugin/usdviewq:/home/carson/git/USD/build/install/lib:/home/carson/opt/embree-3.2.0.x86_64.linux/lib:/home/carson/git/ospray/build/install/lib64:"
         NEW_RPATH "$ORIGIN/../../lib:/home/carson/git/USD/build/install/lib:/home/carson/opt/embree-3.2.0.x86_64.linux/lib:/home/carson/git/ospray/build/install/lib64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/pxr/imaging/hdOSPRay" TYPE FILE FILES
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/context.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderParam.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/config.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/instancer.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/mesh.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/rendererPlugin.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderDelegate.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/renderPass.h"
    "/home/carson/git/hdOSPRayPlugin/pxr/imaging/plugin/hdOSPRay/sampler.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/plugin/usd/hdOSPRay/resources" TYPE FILE RENAME "plugInfo.json" FILES "/home/carson/git/hdOSPRayPlugin/build/pxr/imaging/plugin/hdOSPRay/plugInfo.json")
endif()

