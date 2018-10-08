# WIP: USD + Intel OSPRay Alpha Build

 ![OSPRay](/ospray_screenshot.jpg?raw=true "OSPRay")

## Overview
This is a fork of the main USD repo with an added plugin for rendering with Intel's OSPRay Framework.  This is an early release WIP plugin for Hydra that supports ray tracing and path tracing up to real-time speeds.  Please note that this is not yet fully polished and you will likely run into issues.  For help building or running please send me an email at carson.brownlee AT the place I work dot com.  

Currently tested with Linux - Centos 7 and Arch.  I have yet to get hydra working with Mac (could not get pyside/pyside2 to utilize core profile correctly, there seems to be a running github issue thread on this).

The relevant code is actually entirely contained in pxr/imaging/plugin/hdOSPRay, however this is currently distributed as a fork to maintain version parity with the tested USD version.  Once the plugin is ready to be more regularly updated with current USD versions it will be distributed as only the plugin directoy.

## Limitations
* This is an early release version under development and has so far only really been tested with one scene (the kitchen).
* The denoiser is currently only built with a cmake option enabled, but this will only really be possible for outside users when we release it.  Email us to inquire about early access to OpenImageDenoise.
* Picking is currently not supported and may brake the rendering view.
* hdLux lights are not supported yet
* usdShade support is not implemented yet
* subdivision surfaces are currently being put in

## Dependencies
* USD's standard dependancies for core and view if you want to compile with usdview.
* OSPRay 1.7.x  (1.6 may work).  1.7 is currently available from the devel branch of OSPRay (https://github.com/ospray/ospray).
* Embree 3.2.x

## Building
* enable PXR_BUILD_OSPRAY_PLUGIN (you may need to disable embree plugin due to different embree versions).
* set ospray_DIR to the directory containing your osprayConfig.cmake.  This can be found in the root directory of the distributed binaries or if you are building and installing from source it can be found in <install>/lib/cmake/ospray-1.7.0/
    
## Running
with the plugin built, select view->Hydra Renderer->OSPRay.

environment variables to set include:
* HDOSPRAY_SAMPLES_PER_FRAME
* HDOSPRAY_SAMPLES_TO_CONVERGENCE
* HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES
* HDOSPRAY_CAMERA_LIGHT_INTENSITY
* HDOSPRAY_USEPATHTRACING

see config.cpp for a more complete list.
