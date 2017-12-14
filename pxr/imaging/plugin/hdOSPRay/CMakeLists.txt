set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdOSPRay)

#if(NOT OSPRAY_FOUND)
#    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: OSPRay")
#    return()
#endif()

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
        ${OSPRAY_LIBRARY}
        ${EMBREE_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        ${OSPRAY_INCLUDE_DIR}

    PUBLIC_CLASSES
        config
        instancer
        mesh
        meshSamplers
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
