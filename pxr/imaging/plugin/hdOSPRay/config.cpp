//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hdOSPRay/config.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdOSPRayConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...
TF_DEFINE_ENV_SETTING(HDOSPRAY_SAMPLES_PER_FRAME, -1,
        "Raytraced samples per pixel per frame (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_SAMPLES_TO_CONVERGENCE, 100,
        "Samples per pixel before we stop rendering (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES, 0,
        "Ambient occlusion samples per camera ray (must be >= 0; a value of 0 disables ambient occlusion)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_FIX_RANDOM_SEED, 0,
        "Should HdOSPRay sampling use a fixed random seed? (values > 0 are true)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_FACE_COLORS, 1,
        "Should HdOSPRay use face colors while rendering?");

TF_DEFINE_ENV_SETTING(HDOSPRAY_CAMERA_LIGHT_INTENSITY, 300,
        "Intensity of the camera light, specified as a percentage of <1,1,1>.");

TF_DEFINE_ENV_SETTING(HDOSPRAY_PRINT_CONFIGURATION, 0,
        "Should HdOSPRay print configuration on startup? (values > 0 are true)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_PATHTRACING, 0,
        "Should HdOSPRay use path tracing");

TF_DEFINE_ENV_SETTING(HDOSPRAY_INIT_ARGS, "",
        "Initialization arguments sent to OSPRay");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_DENOISER, 0,
        "OSPRay uses denoiser");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_CHECKERBOARDING, 0,
        "OSPRay uses checkerboarding");

HdOSPRayConfig::HdOSPRayConfig()
{
    // Read in values from the environment, clamping them to valid ranges.
    samplesPerFrame = std::max(-1,
            TfGetEnvSetting(HDOSPRAY_SAMPLES_PER_FRAME));
    samplesToConvergence = std::max(1,
            TfGetEnvSetting(HDOSPRAY_SAMPLES_TO_CONVERGENCE));
    ambientOcclusionSamples = std::max(0,
            TfGetEnvSetting(HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES));
    fixRandomSeed = (TfGetEnvSetting(HDOSPRAY_FIX_RANDOM_SEED) > 0);
    useFaceColors = (TfGetEnvSetting(HDOSPRAY_USE_FACE_COLORS) > 0);
    cameraLightIntensity = (std::max(100,
            TfGetEnvSetting(HDOSPRAY_CAMERA_LIGHT_INTENSITY)) / 100.0f);
    usePathTracing =TfGetEnvSetting(HDOSPRAY_USE_PATHTRACING);
    initArgs =TfGetEnvSetting(HDOSPRAY_INIT_ARGS);
    useDenoiser = TfGetEnvSetting(HDOSPRAY_USE_DENOISER);
    useCheckerboarding = TfGetEnvSetting(HDOSPRAY_USE_CHECKERBOARDING);

    if (TfGetEnvSetting(HDOSPRAY_PRINT_CONFIGURATION) > 0) {
        std::cout
            << "HdOSPRay Configuration: \n"
            << "  samplesPerFrame            = "
            <<    samplesPerFrame         << "\n"
            << "  samplesToConvergence       = "
            <<    samplesToConvergence    << "\n"
            << "  ambientOcclusionSamples    = "
            <<    ambientOcclusionSamples << "\n"
            << "  fixRandomSeed              = "
            <<    fixRandomSeed           << "\n"
            << "  useFaceColors              = "
            <<    useFaceColors           << "\n"
            << "  cameraLightIntensity      = "
            <<    cameraLightIntensity   << "\n"
            << "  initArgs                  = "
            <<    initArgs   << "\n"
            ;
    }
}

/*static*/
const HdOSPRayConfig&
HdOSPRayConfig::GetInstance()
{
    return TfSingleton<HdOSPRayConfig>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
