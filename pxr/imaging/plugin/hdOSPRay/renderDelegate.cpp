//
// Copyright 2018 Intel
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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdOSPRay/renderDelegate.h"

#include "pxr/imaging/hdOSPRay/config.h"
#include "pxr/imaging/hdOSPRay/instancer.h"
#include "pxr/imaging/hdOSPRay/renderParam.h"
#include "pxr/imaging/hdOSPRay/renderPass.h"

#include "pxr/imaging/hd/resourceRegistry.h"

#include "pxr/imaging/hdOSPRay/mesh.h"
#include "pxr/imaging/hdOSPRay/material.h"
//XXX: Add other Rprim types later
#include "pxr/imaging/hd/camera.h"
//XXX: Add other Sprim types later
#include "pxr/imaging/hd/bprim.h"
//XXX: Add bprim types
#include "pxr/imaging/hdSt/material.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdOSPRayTokens, HDOSPRAY_TOKENS);

const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->material,
};

const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
//    HdPrimTypeTokens->texture,
};

std::mutex HdOSPRayRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdOSPRayRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdOSPRayRenderDelegate::_resourceRegistry;


HdOSPRayRenderDelegate::HdOSPRayRenderDelegate()
{
  int ac=1;
  std::string initArgs = HdOSPRayConfig::GetInstance().initArgs;
  std::stringstream ss(initArgs);
  std::string arg;
  std::vector<std::string> args;
  while (ss >> arg)
  {
    args.push_back(arg);
  }
  ac = static_cast<int>(args.size()+1);
  const char** av = new const char*[ac];
  av[0] = "ospray";
  for(int i=1;i < ac; i++) {
    av[i] = args[i - 1].c_str();
  }
  try {
      int init_error = ospInit(&ac,av);
      if (init_error != OSP_NO_ERROR) {
        std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
      } else {
        auto device = ospGetCurrentDevice();
        if (device == nullptr) {
            std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
        }

        ospDeviceSetStatusFunc(device, [](const char *msg) { std::cout << msg; });
        ospDeviceSetErrorFunc(device, [](OSPError e, const char *msg) {
            std::cerr << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
        });

        ospDeviceCommit(device);
      }
  }
  catch (std::runtime_error e) {
    std::cerr << "OSPRAY Initialization error.  Likely incorrect initArgs\n";
    //todo: request addition of ospFinalize() to ospray
  }
  if (ospGetCurrentDevice() == nullptr)
  {
    //user most likely specified bad arguments, retry without them
    ac = 1;
    ospInit(&ac, av);
  }
  delete [] av;

  _model = ospNewModel();
  ospCommit(_model);
  if (HdOSPRayConfig::GetInstance().usePathTracing == 1)
    _renderer = ospNewRenderer("pt");
  else
    _renderer = ospNewRenderer("sv");

    // Store top-level embree objects inside a render param that can be
    // passed to prims during Sync().
    _renderParam =
        std::make_shared<HdOSPRayRenderParam>(_model, _renderer, &_sceneVersion);


    // Initialize one resource registry for all embree plugins
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry.reset( new HdResourceRegistry() );
    }

}

HdOSPRayRenderDelegate::~HdOSPRayRenderDelegate()
{
    // Clean the resource registry only when it is the last Embree delegate
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_sub(1) == 1) {
        _resourceRegistry.reset();
    }

    _renderParam.reset();
}

HdRenderParam*
HdOSPRayRenderDelegate::GetRenderParam() const
{
    return _renderParam.get();
}

void
HdOSPRayRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    // CommitResources() is called after prim sync has finished, but before any
    // tasks (such as draw tasks) have run.
}

TfToken HdOSPRayRenderDelegate::GetMaterialNetworkSelector() const {
  //Carson: this should be "HdOSPRayTokens->ospray", but we return glslfx so that we work with many supplied shaders
  // TODO: is it possible to return both?
  return HdOSPRayTokens->glslfx;
}

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
HdOSPRayRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdAovDescriptor
HdOSPRayRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(HdFormatUNorm8Vec4, true,
                               VtValue(GfVec4f(0.0f)));
    } else if (name == HdAovTokens->normal || name == HdAovTokens->Neye) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false,
                               VtValue(GfVec3f(-1.0f)));
    } else if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name == HdAovTokens->linearDepth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(0.0f));
    } else if (name == HdAovTokens->primId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(0));
    } else {
        HdParsedAovToken aovId(name);
        if (aovId.isPrimvar) {
            return HdAovDescriptor(HdFormatFloat32Vec3, false,
                                   VtValue(GfVec3f(0.0f)));
        }
    }

    return HdAovDescriptor();
}

HdRenderPassSharedPtr
HdOSPRayRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                            HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
        new HdOSPRayRenderPass(index, collection, _model, _renderer, &_sceneVersion));
}

HdInstancer *
HdOSPRayRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                        SdfPath const& id,
                                        SdfPath const& instancerId)
{
    return new HdOSPRayInstancer(delegate, id, instancerId);
}

void
HdOSPRayRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdOSPRayRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdOSPRayMesh(rprimId, instancerId);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdOSPRayRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
HdOSPRayRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
      return new HdCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->material) {
      return new HdOSPRayMaterial(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
HdOSPRayRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    // For fallback sprims, create objects with an empty scene path.
    // They'll use default values and won't be updated by a scene delegate.
    return CreateSprim(typeId, SdfPath::EmptyPath());
}

void
HdOSPRayRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
HdOSPRayRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    return nullptr;
}

HdBprim *
HdOSPRayRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    return CreateBprim(typeId, SdfPath::EmptyPath());
}

void
HdOSPRayRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

PXR_NAMESPACE_CLOSE_SCOPE
