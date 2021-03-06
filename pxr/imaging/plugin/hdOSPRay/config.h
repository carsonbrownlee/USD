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
#ifndef HDOSPRAY_CONFIG_H
#define HDOSPRAY_CONFIG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdOSPRayConfig
///
/// This class is a singleton, holding configuration parameters for HdOSPRay.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdOSPRay takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HDOSPRAY_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdOSPRayConfig {
public:
    /// \brief Return the configuration singleton.
    static const HdOSPRayConfig &GetInstance();

    /// How many samples does each pixel get per frame?
    ///
    /// Override with *HDOSPRAY_SAMPLES_PER_FRAME*.
    unsigned int samplesPerFrame;

    /// How many samples do we need before a pixel is considered
    /// converged?
    ///
    /// Override with *HDOSPRAY_SAMPLES_TO_CONVERGENCE*.
    unsigned int samplesToConvergence;

    /// How many ambient occlusion rays should we generate per
    /// camera ray?
    ///
    /// Override with *HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES*.
    unsigned int ambientOcclusionSamples;

    /// Should the renderpass's sampling functions use a fixed random seed?
    /// (Helpful for things like unit tests, to get consistent results).
    ///
    /// Override with *HDOSPRAY_FIX_RANDOM_SEED*. Integer values greater than
    /// zero are considered "true".
    bool fixRandomSeed;

    /// Should the renderpass use the color primvar, or flat white colors?
    /// (Flat white shows off ambient occlusion better).
    ///
    /// Override with *HDOSPRAY_USE_FACE_COLORS*. Integer values greater than
    /// zero are considered "true".
    bool useFaceColors;

    /// What should the intensity of the camera light be, specified as a
    /// percent of <1, 1, 1>.  For example, 300 would be <3, 3, 3>.
    ///
    /// Override with *HDOSPRAY_CAMERA_LIGHT_INTENSITY*.
    float cameraLightIntensity;

    ///  Whether OSPRay uses path tracing or scivis renderer.
    ///
    /// Override with *HDOSPRAY_USE_PATHTRACING*.
    bool usePathTracing;

    ///  Whether OSPRay uses denoiser
    ///
    /// Override with *HDOSPRAY_USE_DENOISER*.
    bool useDenoiser;

    ///  Whether OSPRay uses checkerboarding
    ///
    /// Override with *HDOSPRAY_USE_CHECKERBOARDING*.
    bool useCheckerboarding;

    /// Initialization arguments sent to OSPRay.
    ///  This can be used to set ospray configurations like mpi.
    ///
    /// Override with *HDOSPRAY_INIT_ARGS*.
    std::string initArgs;
private:
    // The constructor initializes the config variables with their
    // default or environment-provided override, and optionally prints
    // them.
    HdOSPRayConfig();
    ~HdOSPRayConfig() = default;

    HdOSPRayConfig(const HdOSPRayConfig&) = delete;
    HdOSPRayConfig& operator=(const HdOSPRayConfig&) = delete;

    friend class TfSingleton<HdOSPRayConfig>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_CONFIG_H
