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

#include "material.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdOSPRay/config.h"
#include "pxr/imaging/hdOSPRay/context.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayMaterialTokens,
    (UsdPreviewSurface)
);

TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayTokens,
    (UsdPreviewSurface)
    (diffuseColor)
    (specularColor)
    (emissiveColor)
    (metallic)
    (roughness)
    (clearcoat)
    (clearcoatRoughness)
    (ior)
    (color)
    (opacity)
    (UsdUVTexture)
    (normal)
    (displacement)
    (file)
    (scale)
    (wrapS)
    (wrapT)
    (repeat)
    (mirror)
);

/// Synchronizes state from the delegate to this object.
void HdOSPRayMaterial::Sync(HdSceneDelegate *sceneDelegate,
                  HdRenderParam   *renderParam,
                  HdDirtyBits     *dirtyBits)
{
  if (*dirtyBits && HdMaterial::DirtyResource) {
    //update material
    std::cout << "update material\n";

    auto networkMapResource = sceneDelegate->GetMaterialResource(GetId());
    auto networkMap = networkMapResource.Get<HdMaterialNetworkMap>();
    HdMaterialNetwork matNetwork;

    //get material network from network map
    TF_FOR_ALL(itr, networkMap.map) {
      auto & network = itr->second;
      TF_FOR_ALL(node, network.nodes) {
        if (node->identifier == HdOSPRayMaterialTokens->UsdPreviewSurface)
        {
          matNetwork = network;
          std::cout << "found material network usdpreviewsurface!\n";
        }
      }
    }

    HdMaterialNode usdPreviewNode;
    TF_FOR_ALL(node, matNetwork.nodes) {
      std::cout << "matNetwork itr\n";
      if (node->identifier == HdOSPRayTokens->UsdPreviewSurface) {
        usdPreviewNode = *node;
        TF_FOR_ALL(param, node->parameters) {
          const auto & name = param->first;
          const auto & value = param->second;
          if (name == HdOSPRayTokens->diffuseColor) {
            std::cout << "found diffuse color!\n";
            diffuseColor = value.Get<GfVec4f>();
          }
          else if (name == HdOSPRayTokens->color)
            std::cout << "found color!\n";
          else if (name == HdOSPRayTokens->opacity) {
            std::cout << "found opacity!\n";
            opacity = value.Get<float>();
          }
        }
        std::cout << "found matNode usdpreviewsurface\n";
      } else if (node->identifier == HdOSPRayTokens->UsdUVTexture) {
        std::cout << "found texture\n!";
        HdOSPRayTexture texture;

        // find texture inputs and outputs
        auto relationships = matNetwork.relationships;
        auto relationship = std::find_if(relationships.begin(), relationships.end(), [&node](HdMaterialRelationship const& rel){
            return rel.inputId == node->path;
        });
        if (relationship == relationships.end())
          continue;  //node isn't actually used

        TF_FOR_ALL(param, node->parameters) {
          const auto & name = param->first;
          const auto & value = param->second;
          if (name == HdOSPRayTokens->file) {
            texture.file = value.Get<SdfAssetPath>().GetAssetPath();
            std::cout << "found texture file: " << texture.file << std::endl;
          } else if (name == HdOSPRayTokens->scale) {
            texture.scale = value.Get<GfVec4f>();
          } else if (name == HdOSPRayTokens->wrapS) {
          } else if (name == HdOSPRayTokens->wrapT) {
          } else {
            std::cout << "unhandled token: " << std::endl;
          }

          TfToken texNameToken = relationship->outputName;
          if (texNameToken == HdOSPRayTokens->diffuseColor)
          {
            std::cout << "found diffuseColor texture\n";
            map_diffuseColor = texture;
          }
        }
      }
    }
   std::string rendererType = HdOSPRayConfig::GetInstance().usePathTracing  ? "pt" : "sv";
   std::string materialType = "default";
   _ospMaterial = ospNewMaterial2(rendererType.c_str(),
                                  materialType.c_str());
   ospSet3fv(_ospMaterial, "Kd", diffuseColor.data());

    *dirtyBits = Clean;
  }
}

PXR_NAMESPACE_CLOSE_SCOPE
