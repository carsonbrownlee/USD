set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdOSPRay)
add_custom_target(shared_libs)

message("hdOSPRayPlugin cmake begin")

option(PXR_BUILD_OSPRAY_PLUGIN "build Intel OSPRay rendering backend")
if (PXR_BUILD_OSPRAY_PLUGIN)
  message("hdOSPRayPlugin building")
  find_package(ospray 1.8 CONFIG REQUIRED)

  if(NOT PXR_BUILD_OPENIMAGEIO_PLUGIN)
    message(FATAL_ERROR "hdOSPRayPlugin requires PXR_BUILD_OPENIMAGEIO_PLUGIN")
  endif()

  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pxr/imaging/plugin/hdOSPRay/cmake)

  option(HDOSPRAY_ENABLE_DENOISER "build OSPRay plugin using OpenImageDenoise")
  if (HDOSPRAY_ENABLE_DENOISER)
    find_package(OpenImageDenoise REQUIRED)
    if (OPENIMAGEDENOISE_FOUND)
      add_definitions(-DHDOSPRAY_ENABLE_DENOISER)
      include_directories(${OPENIMAGEDENOISE_INCLUDE_DIRS})
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
      ${TBB_LIBRARIES}
      ${OSPRAY_LIBRARIES}
      ${EMBREE_LIBRARIES}
      ${OPENIMAGEDENOISE_LIBRARIES}
      ${OIIO_LIBRARIES}

    INCLUDE_DIRS
      ${TBB_INCLUDE_DIRS}
      ${OSPRAY_INCLUDE_DIRS}
      ${OIIO_INCLUDE_DIRS}

    PUBLIC_CLASSES
      config
      instancer
      mesh
      material
      texture
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
