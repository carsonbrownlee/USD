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

#include "config.h"
#include "context.h"

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayMaterialTokens,
    (UsdPreviewSurface)
    (HwPtexTexture_1)
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
    (HwPtexTexture_1)
    (normal)
    (displacement)
    (file)
    (filename)
    (scale)
    (wrapS)
    (wrapT)
    (repeat)
    (mirror)
);

OSPTextureFormat
osprayTextureFormat(int depth, int channels, bool preferLinear = false)
{
  if (depth == 1) {
    if( channels == 1 ) return OSP_TEXTURE_R8;
    if( channels == 3 )
      return preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
    if( channels == 4 )
      return preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
  } else if (depth == 4) {
    if( channels == 1 ) return OSP_TEXTURE_R32F;
    if( channels == 3 ) return OSP_TEXTURE_RGB32F;
    if( channels == 4 ) return OSP_TEXTURE_RGBA32F;
  }

  return OSP_TEXTURE_FORMAT_INVALID;
}

OSPTexture LoadPtexTexture(std::string file)
{
  std::cout << "loading ptex file " << file << std::endl;
  OSPTexture ospTexture = ospNewTexture("ptex");
  ospSetString(ospTexture, "filename", file.c_str());
  ospCommit(ospTexture);
  assert(ospTexture);
  return ospTexture;
}

OSPTexture LoadOIIOTexture2D(std::string file, bool nearestFilter=false)
{
  ImageInput *in = ImageInput::open(file.c_str());
  if (!in) {
    std::cerr << "#osp: failed to load texture '"+file+"'" << std::endl;
    return nullptr;
  }

  const ImageSpec &spec = in->spec();
  osp::vec2i size;
  size.x = spec.width;
  size.y = spec.height;
  int channels = spec.nchannels;
  const bool hdr = spec.format.size() > 1;
  int depth = hdr ? 4 : 1;
  const size_t stride = size.x * channels * depth;
  unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * size.y * stride);

  in->read_image(hdr ? TypeDesc::FLOAT : TypeDesc::UINT8, data);
  in->close();
  ImageInput::destroy(in);

  // flip image (because OSPRay's textures have the origin at the lower left corner)
  for (int y = 0; y < size.y / 2; y++) {
    unsigned char *src = &data[y * stride];
    unsigned char *dest = &data[(size.y-1-y) * stride];
    for (size_t x = 0; x < stride; x++)
      std::swap(src[x], dest[x]);
  }

  OSPData ospData = ospNewData(stride*size.y, OSP_UCHAR, data);
  ospCommit(ospData);
//  delete data;
//  data = nullptr;

  OSPTexture ospTexture = ospNewTexture("texture2d");
  ospSet1i(ospTexture, "type", (int)osprayTextureFormat(depth, channels));
  ospSet1i(ospTexture, "flags", nearestFilter ? OSP_TEXTURE_FILTER_NEAREST : 0);
  ospSet2i(ospTexture, "size", size.x, size.y);
  ospSetData(ospTexture, "data", ospData);

  ospCommit(ospTexture);
  assert(ospTexture);
  if (ospTexture)
    std::cout << "sucessfully created ospTexture\n";
  return ospTexture;
}

HdOSPRayMaterial::HdOSPRayMaterial(SdfPath const& id)
        : HdMaterial(id)
{
        diffuseColor = GfVec4f(1,1,1,1);
}

/// Synchronizes state from the delegate to this object.
void HdOSPRayMaterial::Sync(HdSceneDelegate *sceneDelegate,
                  HdRenderParam   *renderParam,
                  HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

  if (*dirtyBits & HdMaterial::DirtyResource) {
    //update material
    std::cout << "update material\n";

    VtValue networkMapResource = sceneDelegate->GetMaterialResource(GetId());
    std::cout << " networkMapResource: " << networkMapResource << std::endl;
    HdMaterialNetworkMap networkMap = networkMapResource.Get<HdMaterialNetworkMap>();
    std::cout << " networkMap: " << networkMap << std::endl;
    HdMaterialNetwork matNetwork;

    if (networkMap.map.empty())
        std::cout << "material network map was empty!!!!!\n";

    //get material network from network map
    TF_FOR_ALL(itr, networkMap.map) {
      std::cout << "checking matnetworkmap with num nodes: " << itr->second.nodes.size() << std::endl;
      auto & network = itr->second;
      TF_FOR_ALL(node, network.nodes) {
        std::cout << "network map node: " << node->identifier << std::endl;
        if (node->identifier == HdOSPRayMaterialTokens->UsdPreviewSurface)
        {
          matNetwork = network;
          std::cout << "found material network usdpreviewsurface!\n";
        }
      }
    }


    HdMaterialNode usdPreviewNode;
    TF_FOR_ALL(node, matNetwork.nodes) {
      std::cout << "matNetwork itr: " << node->identifier.GetString() << "\n";
      if (node->identifier == HdOSPRayTokens->UsdPreviewSurface) {
        usdPreviewNode = *node;
        TF_FOR_ALL(param, node->parameters) {
          const auto & name = param->first;
          const auto & value = param->second;
          std::cout << "preview node param: " << name.GetString() << std::endl;
          if (name == HdOSPRayTokens->diffuseColor) {
            diffuseColor = value.Get<GfVec4f>();
          } else if (name == HdOSPRayTokens->metallic) {
            metallic = value.Get<float>();
          } else if (name == HdOSPRayTokens->roughness) {
            roughness = value.Get<float>();
          } else if (name == HdOSPRayTokens->ior) {
            ior = value.Get<float>();
          } else if (name == HdOSPRayTokens->color) {
            diffuseColor = value.Get<GfVec4f>();
          } else if (name == HdOSPRayTokens->opacity) {
            opacity = value.Get<float>();
          }
        }
        std::cout << "found matNode usdpreviewsurface\n";
      } else if (node->identifier == HdOSPRayTokens->UsdUVTexture
                 || node->identifier == HdOSPRayTokens->HwPtexTexture_1) {
        std::cout << "found texture!\n";
        bool isPtex = node->identifier == HdOSPRayTokens->HwPtexTexture_1;
        if (isPtex) {
          std::cout << "found ptex texture!\n";
        }

        // find texture inputs and outputs
        auto relationships = matNetwork.relationships;
        auto relationship = std::find_if(relationships.begin(), relationships.end(), [&node](HdMaterialRelationship const& rel){
            return rel.inputId == node->path;
        });
        if (relationship == relationships.end())
        {
          std::cout << "mat node was not in relationship\n";
          continue;  //node isn't actually used
        }

        HdOSPRayTexture texture;
        TF_FOR_ALL(param, node->parameters) {
          const auto & name = param->first;
          const auto & value = param->second;
          std::cout << "texture node param: " << name.GetString() << std::endl;
          if (name == HdOSPRayTokens->file) {
            SdfAssetPath const& path = value.Get<SdfAssetPath>();
            texture.file = path.GetResolvedPath();
            std::cout << "found texture file: " << texture.file << std::endl;
            texture.ospTexture = LoadOIIOTexture2D(texture.file);
          } else if (name == HdOSPRayTokens->filename) {
            SdfAssetPath const& path = value.Get<SdfAssetPath>();
            texture.file = path.GetResolvedPath();
            std::cout << "found ptex texture file: " << texture.file << std::endl;
            if (isPtex) {
              texture.isPtex = true;
              texture.ospTexture = LoadPtexTexture(texture.file);
            }
          } else if (name == HdOSPRayTokens->scale) {
            texture.scale = value.Get<GfVec4f>();
            std::cout << "found texture scale: "  << texture.scale << std::endl;
          } else if (name == HdOSPRayTokens->wrapS) {
          } else if (name == HdOSPRayTokens->wrapT) {
          } else {
            std::cout << "unhandled token: " << name.GetString() << " " << std::endl;
          }
        }

        TfToken texNameToken = relationship->outputName;
        if (texNameToken == HdOSPRayTokens->diffuseColor)
        {
          std::cout << "found diffuseColor texture\n";
          map_diffuseColor = texture;
        } else if (texNameToken == HdOSPRayTokens->metallic)
        {
          map_metallic = texture;
          std::cout << "found metallic texture\n";
        } else if (texNameToken == HdOSPRayTokens->roughness)
        {
          map_roughness = texture;
          std::cout << "found roughness texture\n";
        } else if (texNameToken == HdOSPRayTokens->normal)
        {
          map_normal = texture;
          std::cout << "found normal texture\n";
        }
        else
          std::cout << "unhandled texToken: " << texNameToken.GetString() << std::endl;

      }
    }
   _ospMaterial = CreateDefaultMaterial(diffuseColor);

   if (map_diffuseColor.ospTexture) {
     ospSetObject(_ospMaterial, "baseColorMap", map_diffuseColor.ospTexture);
     ospSetObject(_ospMaterial, "map_Kd", map_diffuseColor.ospTexture);
   }
   if (map_metallic.ospTexture) {
     ospSetObject(_ospMaterial, "metallicMap", map_metallic.ospTexture);
     std::cout << "setting metallic\n";
     metallic = 1.0f;
   }
   if (map_roughness.ospTexture) {
     ospSetObject(_ospMaterial, "roughnessMap", map_roughness.ospTexture);
     roughness = 1.0f;
   }
   if (map_roughness.ospTexture) {
     ospSetObject(_ospMaterial, "normalMap", map_normal.ospTexture);
     normal = 1.f;
   }
   ospSet1f(_ospMaterial, "ior", ior);
   ospSet3fv(_ospMaterial, "baseColor", diffuseColor.data());
   ospSet1f(_ospMaterial, "metallic", metallic);
   ospSet1f(_ospMaterial, "roughness", roughness);
   ospSet1f(_ospMaterial, "normal", normal);

   ospCommit(_ospMaterial);

    *dirtyBits = Clean;
  }


}

HdStTextureResourceSharedPtr
HdOSPRayMaterial::_GetTextureResource(
        HdSceneDelegate *sceneDelegate,
        HdMaterialParam const &param)
{
    HdResourceRegistrySharedPtr const &resourceRegistry =
        sceneDelegate->GetRenderIndex().GetResourceRegistry();

    HdStTextureResourceSharedPtr texResource;

    SdfPath const &connection = param.GetConnection();
    if (!connection.IsEmpty()) {
        HdTextureResource::ID texID =
            GetTextureResourceID(sceneDelegate, connection);

        if (texID != HdTextureResource::ID(-1)) {

            // Use render index to convert local texture id into global
            // texture key
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdResourceRegistry::TextureKey texKey =
                                               renderIndex.GetTextureKey(texID);

            HdInstance<HdResourceRegistry::TextureKey,
                        HdTextureResourceSharedPtr> texInstance;

            bool textureResourceFound = false;
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->FindTextureResource
                                  (texKey, &texInstance, &textureResourceFound);

            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and continue onto the
            // next param.
            if (!textureResourceFound) {
                TF_WARN("No texture resource found with path %s",
                    param.GetConnection().GetText());
            } else {
                texResource =
                    boost::dynamic_pointer_cast<HdStTextureResource>
                    (texInstance.GetValue());
            }
        }
    }

    // There are many reasons why texResource could be null here:
    // - A missing or invalid connection path,
    // - A deliberate (-1) or accidental invalid texture id
    // - Scene delegate failed to return a texture resource (due to asset error)
    //
    // In all these cases fallback to a simple texture with the provided
    // fallback value
    //
    // XXX todo handle fallback Ptex textures
    if (!texResource) {
        // Fallback texture are only supported for UV textures.
        if (param.GetTextureType() != HdTextureType::Uv) {
            return {};
        }
//        GlfUVTextureStorageRefPtr texPtr =
//            GlfUVTextureStorage::New(1,1, param.GetFallbackValue());
//        GlfTextureHandleRefPtr texture =
//            GlfTextureRegistry::GetInstance().GetTextureHandle(texPtr);
//        texResource.reset(
//            new HdStSimpleTextureResource(texture,
//                                          HdTextureType::Uv,
//                                          HdWrapClamp,
//                                          HdWrapClamp,
//                                          HdMinFilterNearest,
//                                          HdMagFilterNearest,
//                                          0));
//        _fallbackTextureResources.push_back(texResource);
    }

    return texResource;
}

OSPMaterial HdOSPRayMaterial::CreateDefaultMaterial(GfVec4f color)
{
   std::string rendererType = HdOSPRayConfig::GetInstance().usePathTracing  ? "pathtracer" : "scivis";
   OSPMaterial ospMaterial;
   if (rendererType == "pathtracer") {
     std::cout << "created pathtracer material\n";
     ospMaterial = ospNewMaterial2(rendererType.c_str(), "Principled");
//     ospSet3fv(ospMaterial,"baseColor",static_cast<float*>(&color.data()[0]));
//     ospSet1f(ospMaterial,"transmission",1.f-color.data()[3]);
//     ospSet1f(ospMaterial,"roughness", 0.1f);
//     ospSet1f(ospMaterial,"specular", 0.1f);
//     ospSet1f(ospMaterial,"metallic", 0.f);
   }
   else {
     std::cout << "created scivis material\n";
     ospMaterial = ospNewMaterial2(rendererType.c_str(), "OBJMaterial");
     //Carson: apparently colors are actually stored as a single color value for entire object
//     ospSetf(ospMaterial,"Ns",10.f);
//     ospSet3f(ospMaterial,"Ks",0.2f,0.2f,0.2f);
//     ospSet3fv(ospMaterial,"Kd",static_cast<float*>(&color[0]));
//     ospSet1f(ospMaterial,"d",color.data()[3]);
   }
   ospCommit(ospMaterial);
   return ospMaterial;
}

PXR_NAMESPACE_CLOSE_SCOPE
