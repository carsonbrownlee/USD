set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdOSPRay)

#if(NOT OSPRAY_FOUND)
#    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: OSPRay")
#    return()
#endif()

option(PXR_BUILD_OSPRAY_PLUGIN "build Intel OSPRay rendering backend")
# --OSPRay
if (PXR_BUILD_OSPRAY_PLUGIN)
  #find_package(embree 3.1.0 CONFIG REQUIRED)
  #set(EMBREE_FOUND DEFINED embree_DIR)
  find_package(ospray 1.7.0 CONFIG REQUIRED)
  set(OSPRAY_FOUND DEFINED ospray_DIR)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pxr/imaging/plugin/hdOSPRay/cmake)

find_package(OpenImageDenoise)
if (OPENIMAGEDENOISE_FOUND)
  add_definitions(-DHDOSPRAY_ENABLE_DENOISER)
  include_directories(${OPENIMAGEDENOISE_INCLUDE_DIRS})
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

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        ${OSPRAY_INCLUDE_DIRS}

    PUBLIC_CLASSES
        config
        instancer
        mesh
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

if (X11_FOUND OR APPLE)
pxr_build_test(testHdOSPRay
    LIBRARIES
        hdOSPRay
        garch
        arch
    CPPFILES
        testenv/testHdOSPRay.cpp
)
endif()
