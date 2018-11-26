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

#ifndef HDOSPRAY_MATERIAL_H
#define HDOSPRAY_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"

#include "ospray/ospray.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdOSPRayMaterial final : public HdMaterial {
public:
  HdOSPRayMaterial(SdfPath const& id)
    : HdMaterial(id)
  {

  }

  virtual ~HdOSPRayMaterial() = default;

    /// Synchronizes state from the delegate to this object.
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
  {
  }

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override
  {
    return AllDirty;
  }

    /// Causes the shader to be reloaded.
    virtual void Reload() override
  {
  }

    /// Summary flag. Returns true if the material is bound to one or more
    /// textures and any of those textures is a ptex texture.
    /// If no textures are bound or all textures are uv textures, then
    /// the method returns false.
  inline bool HasPtex() const { return false; }

  inline const OSPMaterial GetOSPRayMaterial() const { return _ospMaterial; }

protected:
  OSPMaterial _ospMaterial{nullptr};
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif HDOSPRAY_MATERIAL_H
