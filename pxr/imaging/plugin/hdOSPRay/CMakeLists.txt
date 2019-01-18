set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdOSPRay)
add_custom_target(shared_libs)

option(PXR_BUILD_OSPRAY_PLUGIN "build Intel OSPRay rendering backend" ON)
if (PXR_BUILD_OSPRAY_PLUGIN)
  find_package(ospray 1.8 CONFIG REQUIRED)

  option(HDOSPRAY_ENABLE_DENOISER "build OSPRay plugin using OpenImageDenoise" OFF)
  if (HDOSPRAY_ENABLE_DENOISER)
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    find_package(OpenImageDenoise REQUIRED)
    if (OPENIMAGEDENOISE_FOUND)
      add_definitions(-DHDOSPRAY_ENABLE_DENOISER)
      include_directories(${OPENIMAGEDENOISE_INCLUDE_DIRS})
      list(APPEND OSPRAY_INCLUDE_DIRS ${OPENIMAGEDENOISE_INCLUDE_DIRS})
      list(APPEND OSPRAY_LIBRARIES ${OPENIMAGEDENOISE_LIBRARIES})
    endif()
  endif()

  pxr_plugin(hdOSPRay
    LIBRARIES
      plug
      tf
      vt
      gf
      work
      hf
      hd
      hdx
      ${OSPRAY_LIBRARIES}

    INCLUDE_DIRS
      ${OSPRAY_INCLUDE_DIRS}

    PUBLIC_CLASSES
      config
      instancer
      mesh
      material
      rendererPlugin
      renderDelegate
      renderPass
      sampler

    PUBLIC_HEADERS
      context.h
      renderParam.h

    RESOURCE_FILES
      plugInfo.json
    )

#  if (X11_FOUND OR APPLE)
#    pxr_build_test(testHdOSPRay
#      LIBRARIES
#        hdOSPRay
#        garch
#        arch
#      CPPFILES
#        testenv/testHdOSPRay.cpp
#      )
#  endif()

endif()
