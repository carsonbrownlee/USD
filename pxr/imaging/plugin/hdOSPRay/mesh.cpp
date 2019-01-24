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
#include "pxr/imaging/hdOSPRay/mesh.h"
#include "pxr/imaging/hdOSPRay/config.h"
#include "pxr/imaging/hdOSPRay/context.h"
#include "pxr/imaging/hdOSPRay/instancer.h"
#include "pxr/imaging/hdOSPRay/material.h"
#include "pxr/imaging/hdOSPRay/renderParam.h"
#include "pxr/imaging/hdOSPRay/renderPass.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/material.h"

#include "ospcommon/AffineSpace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayTokens,
    (st)
);

std::mutex g_mutex;

HdOSPRayMesh::HdOSPRayMesh(SdfPath const& id,
                           SdfPath const& instancerId)
        : HdMesh(id, instancerId)
        , _ospMesh(nullptr)
        , _adjacencyValid(false)
        , _normalsValid(false)
        , _refined(false)
        , _smoothNormals(false)
        , _doubleSided(false)
        , _cullStyle(HdCullStyleDontCare)
{
}

void
HdOSPRayMesh::Finalize(HdRenderParam *renderParam)
{
  OSPModel scene = static_cast<HdOSPRayRenderParam*>(renderParam)
                  ->GetOSPRayModel();
  _ospInstances.clear();
}

HdDirtyBits
HdOSPRayMesh::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateMesh(), so it should list every data item
    // that _PopulateMesh requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::AllDirty  //this magic bit seems to trigger materials... bah
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdOSPRayMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
  return bits;
}


void
HdOSPRayMesh::_InitRepr(TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(dirtyBits);

    // Create an empty repr.
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    if (it == _reprs.end()) {
        _reprs.emplace_back(reprToken, HdReprSharedPtr());
    }
}

void
HdOSPRayMesh::Sync(HdSceneDelegate* sceneDelegate,
                   HdRenderParam     *renderParam,
                   HdDirtyBits       *dirtyBits,
                   TfToken const &reprToken)
{
  HD_TRACE_FUNCTION();
  HF_MALLOC_TAG_FUNCTION();

    // The repr token is used to look up an HdMeshReprDesc struct, which
    // has drawing settings for this prim to use. Repr opinions can come
    // from the render pass's rprim collection or the scene delegate;
    // _GetReprName resolves these multiple opinions.
    // HdReprSelector calculatedReprToken = _GetReprSelector(reprToken, forcedRepr);

  // XXX: Meshes can have multiple reprs; this is done, for example, when
  // the drawstyle specifies different rasterizing modes between front faces
  // and back faces. With raytracing, this concept makes less sense, but
  // combining semantics of two HdMeshReprDesc is tricky in the general case.
  // For now, HdOSPRayMesh only respects the first desc; this should be fixed.
  _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdMeshReprDesc &desc = descs[0];

  OSPModel model = static_cast<HdOSPRayRenderParam*>(renderParam)->GetOSPRayModel();
  OSPRenderer renderer = static_cast<HdOSPRayRenderParam*>(renderParam)->GetOSPRayRenderer();

  if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
    _SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
                   sceneDelegate->GetMaterialId(GetId()));
  }

  // Create ospray geometry objects.
  _PopulateOSPMesh(sceneDelegate, model, renderer, dirtyBits, desc);

  if (*dirtyBits & HdChangeTracker::DirtyTopology)
  {
    //TODO: update material here?
  }
}

void
HdOSPRayMesh::_UpdatePrimvarSources(HdSceneDelegate* sceneDelegate,
                                    HdDirtyBits dirtyBits)
{
  HD_TRACE_FUNCTION();
  SdfPath const& id = GetId();

    // Update _primvarSourceMap, our local cache of raw primvar data.
    // This function pulls data from the scene delegate, but defers processing.
    //
    // While iterating primvars, we skip "points" (vertex positions) because
    // the points primvar is processed by _PopulateMesh. We only call
    // GetPrimvar on primvars that have been marked dirty.
    //
    // Currently, hydra doesn't have a good way of communicating changes in
    // the set of primvars, so we only ever add and update to the primvar set.

    HdPrimvarDescriptorVector primvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) {
        HdInterpolation interp = static_cast<HdInterpolation>(i);
        primvars = GetPrimvarDescriptors(sceneDelegate, interp);
        for (HdPrimvarDescriptor const& pv: primvars) {

          //test texcoords
          if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, HdOSPRayTokens->st)) {
            auto value = sceneDelegate->Get(id, pv.name);
            if (value.IsHolding<VtVec2fArray>()) {
              _texcoords = value.Get<VtVec2fArray>();
            }
           }

            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name) &&
                pv.name != HdTokens->points) {
                _primvarSourceMap[pv.name] = {
                    GetPrimvar(sceneDelegate, pv.name),
                    interp
                };
            }
        }
    }
}

void
HdOSPRayMesh::_PopulateOSPMesh(HdSceneDelegate* sceneDelegate,
                              OSPModel model,
                              OSPRenderer renderer,
                              HdDirtyBits*     dirtyBits,
                              HdMeshReprDesc const &desc)
{
  HD_TRACE_FUNCTION();
  HF_MALLOC_TAG_FUNCTION();

  SdfPath const& id = GetId();

    ////////////////////////////////////////////////////////////////////////
    // 1. Pull scene data.

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        VtValue value = sceneDelegate->Get(id, HdTokens->points);
        if (value.IsHolding<VtVec3fArray>())
          _points = value.Get<VtVec3fArray>();
        _normalsValid = false;
    }

    if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
        HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);
        _topology = HdMeshTopology(_topology,
            displayStyle.refineLevel);
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _transform = GfMatrix4f(sceneDelegate->GetTransform(id));
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _UpdateVisibility(sceneDelegate, dirtyBits);
    }

    if (HdChangeTracker::IsCullStyleDirty(*dirtyBits, id)) {
        _cullStyle = GetCullStyle(sceneDelegate);
    }
    if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id)) {
        _doubleSided = IsDoubleSided(sceneDelegate);
    }
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->widths) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdOSPRayTokens->st)
        ) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    ////////////////////////////////////////////////////////////////////////
    // 2. Resolve drawstyles

    // The repr defines a set of geometry styles for drawing the mesh
    // (see hd/enums.h). We're ignoring points and wireframe for now, so
    // HdMeshGeomStyleSurf maps to subdivs and everything else maps to
    // HdMeshGeomStyleHull (coarse triangulated mesh).
    bool doRefine = (desc.geomStyle == HdMeshGeomStyleSurf);

    // The repr defines whether we should compute smooth normals for this mesh:
    // per-vertex normals taken as an average of adjacent faces, and
    // interpolated smoothly across faces.
    _smoothNormals = !desc.flatShadingEnabled;


  ////////////////////////////////////////////////////////////////////////
  // 3. Populate ospray prototype object.

  // If the topology has changed, or the value of doRefine has changed, we
  // need to create or recreate the OSPRay mesh object.
  // _GetInitialDirtyBits() ensures that the topology is dirty the first time
  // this function is called, so that the OSPRay mesh is always created.
  bool newMesh = false;
  if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
                  doRefine != _refined) {

    newMesh = true;

    // Force the smooth normals code to rebuild the "normals" primvar the
    // next time smooth normals is enabled.
    _normalsValid = false;
  }

  if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
    // When pulling a new topology, we don't want to overwrite the
    // refine level or subdiv tags, which are provided separately by the
    // scene delegate, so we save and restore them.
    PxOsdSubdivTags subdivTags = _topology.GetSubdivTags();
    int refineLevel = _topology.GetRefineLevel();
    _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), refineLevel);
    _topology.SetSubdivTags(subdivTags);
    _adjacencyValid = false;
  }

  if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) &&
                  _topology.GetRefineLevel() > 0) {
    _topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
  }

  // Update the smooth normals in steps:
  // 1. If the topology is dirty, update the adjacency table, a processed
  //    form of the topology that helps calculate smooth normals quickly.
  // 2. If the points are dirty, update the smooth normal buffer itself.
  _normalsValid = false;
  if (_smoothNormals && !_adjacencyValid) {
    _adjacency.BuildAdjacencyTable(&_topology);
    _adjacencyValid = true;
    // If we rebuilt the adjacency table, force a rebuild of normals.
    _normalsValid = false;
  }
  if (_smoothNormals && !_normalsValid) {
        _computedNormals = Hd_SmoothNormals::ComputeSmoothNormals(
            &_adjacency, _points.size(), _points.cdata());
    _normalsValid = true;
  }

  std::lock_guard<std::mutex> lock(g_mutex);
  // Create new OSP Mesh
  auto instanceModel = ospNewModel();
  if (newMesh ||
                  HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points) ||
                  HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdOSPRayTokens->st) )
  {

    if (_primvarSourceMap.count(HdTokens->color) > 0) {
      auto& colorBuffer = _primvarSourceMap[HdTokens->color].data;
      if (colorBuffer.GetArraySize() && colorBuffer.IsHolding<VtVec4fArray>())
        _colors = colorBuffer.Get<VtVec4fArray>();
    }

    bool useQuads = true;
    OSPGeometry mesh = nullptr;
    if (useQuads) {
      mesh = ospNewGeometry("quadmesh");

      HdMeshUtil meshUtil(&_topology, GetId());
      meshUtil.ComputeQuadIndices(&_quadIndices,
                                      &_quadPrimitiveParams);


      auto indices = ospNewData(_quadIndices.size(), OSP_INT4,
                                _quadIndices.cdata(), OSP_DATA_SHARED_BUFFER);

      ospCommit(indices);
      ospSetData(mesh, "index", indices);
      ospRelease(indices);

      TfToken buffName = HdOSPRayTokens->st;
      VtValue buffValue = VtValue(_texcoords);
      HdVtBufferSource buffer(buffName, buffValue);
      VtValue quadPrimvar;
      if (!meshUtil.ComputeQuadrangulatedFaceVaryingPrimvar(buffer.GetData(), buffer.GetNumElements(),
                                                 buffer.GetTupleType().type, &quadPrimvar)) {
        std::cout << "ERROR: could not triangule face-varying data\n";
      } else {
        if (quadPrimvar.IsHolding<VtVec2fArray>())
        _texcoords = quadPrimvar.Get<VtVec2fArray>();
      }

      //usd stores texcoords in face indexed -> each quad has 4 unique texcoords.
      // let's try converting it to match our vertex indices
      VtVec2fArray texcoords2;
      texcoords2.resize(_points.size());
      for (size_t q = 0; q < _quadIndices.size(); q++) {
        for (int i = 0; i < 4; i++) {
          // value at quadindex[i][q] maps to i*4+q texcoord;
          const size_t tc1index = q*4+i;
          const size_t tc2index = _quadIndices[q][i];
          if (tc1index < _texcoords.size() && tc2index < texcoords2.size())
            texcoords2[tc2index] = _texcoords[tc1index];
        }
      }
      _texcoords = texcoords2;

    } else {  //triangles
      mesh = ospNewGeometry("trianglemesh");

      HdMeshUtil meshUtil(&_topology, GetId());
      meshUtil.ComputeTriangleIndices(&_triangulatedIndices,
                                      &_trianglePrimitiveParams);


      auto indices = ospNewData(_triangulatedIndices.size(), OSP_INT3,
                                _triangulatedIndices.cdata(), OSP_DATA_SHARED_BUFFER);

      ospCommit(indices);
      ospSetData(mesh, "index", indices);
      ospRelease(indices);

      TfToken buffName = HdOSPRayTokens->st;
      VtValue buffValue = VtValue(_texcoords);
      HdVtBufferSource buffer(buffName, buffValue);
      VtValue triangulatedPrimvar;
      if (!meshUtil.ComputeTriangulatedFaceVaryingPrimvar(buffer.GetData(), buffer.GetNumElements(),
                                                 buffer.GetTupleType().type, &triangulatedPrimvar)) {
        std::cout << "ERROR: could not triangule face-varying data\n";
      } else {
        if (triangulatedPrimvar.IsHolding<VtVec2fArray>())
        _texcoords = triangulatedPrimvar.Get<VtVec2fArray>();
      }
    }

    auto vertices = ospNewData(_points.size(),OSP_FLOAT3, _points.cdata(),
                               OSP_DATA_SHARED_BUFFER);
    ospCommit(vertices);
    ospSetData(mesh, "vertex", vertices);
    ospRelease(vertices);

    if (_computedNormals.size()) {
      auto normals = ospNewData(_computedNormals.size(),OSP_FLOAT3,
                                _computedNormals.cdata(), OSP_DATA_SHARED_BUFFER);
      ospSetData(mesh, "vertex.normal", normals);
      ospRelease(normals);
    }

    if (_colors.size() > 1) {
      //Carson: apparently colors are actually stored as a single color value for entire object
      auto colors = ospNewData(_colors.size(),OSP_FLOAT4, _colors.cdata(),
                               OSP_DATA_SHARED_BUFFER);
      ospSetData(mesh, "vertex.color", colors);
      ospRelease(colors);
    }

    if (_texcoords.size() > 1) {
      auto texcoords = ospNewData(_texcoords.size(),OSP_FLOAT2, _texcoords.cdata(),
               OSP_DATA_SHARED_BUFFER);
      ospCommit(texcoords);
      ospSetData(mesh, "vertex.texcoord", texcoords);
      ospRelease(texcoords);
    }

    OSPMaterial ospMaterial = nullptr;
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    const HdOSPRayMaterial *material = static_cast<const HdOSPRayMaterial *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));

    if (material && material->GetOSPRayMaterial()) {
      ospMaterial = material->GetOSPRayMaterial();
    } else {
      //Create new ospMaterial
      GfVec4f color(1.f);
      if (!_colors.empty())
        color = _colors[0];
      ospMaterial = HdOSPRayMaterial::CreateDefaultMaterial(color);
    }

    ospCommit(ospMaterial);
    ospSetMaterial(mesh, ospMaterial);
    ospRelease(ospMaterial);
    ospCommit(mesh);

    ospAddGeometry(instanceModel, mesh); // crashing when added to the scene. I suspect indices/vertex spec.
    ospCommit(instanceModel);
  }

  ////////////////////////////////////////////////////////////////////////
  // 4. Populate ospray instance objects.

  // If the mesh is instanced, create one new instance per transform.
  // XXX: The current instancer invalidation tracking makes it hard for
  // HdOSPRay to tell whether transforms will be dirty, so this code
  // pulls them every frame.
  if (!GetInstancerId().IsEmpty()) {
    // Retrieve instance transforms from the instancer.
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdInstancer *instancer =
                    renderIndex.GetInstancer(GetInstancerId());
    VtMatrix4dArray transforms =
                    static_cast<HdOSPRayInstancer*>(instancer)->
                    ComputeInstanceTransforms(GetId());

    size_t oldSize = _ospInstances.size();
    size_t newSize = transforms.size();

    // Size down (if necessary).
    for(size_t i = newSize; i < oldSize; ++i) {
      for (auto instance : _ospInstances) {
        ospRemoveGeometry(model, instance);
      }
    }
    _ospInstances.resize(newSize);

    // Size up (if necessary).
    for(size_t i = oldSize; i < newSize; ++i) {
      // Create the new instance.
      _ospInstances[i] = ospNewInstance(instanceModel, (osp::affine3f&)ospcommon::one);
    }

    // Update transforms.
    for (size_t i = 0; i < _ospInstances.size(); ++i) {
      auto instance = _ospInstances[i];
      // Combine the local transform and the instance transform.
      GfMatrix4f matf = _transform * GfMatrix4f(transforms[i]);
      float* xfm = matf.GetArray();
      //convert aligned matrix to unalighned 4x3 matrix
      ospSet3f(instance,"xfm.l.vx",xfm[0],xfm[1],xfm[2]);
      ospSet3f(instance,"xfm.l.vy",xfm[4],xfm[5],xfm[6]);
      ospSet3f(instance,"xfm.l.vz",xfm[8],xfm[9],xfm[10]);
      ospSet3f(instance,"xfm.p",xfm[12],xfm[13],xfm[14]);
      ospCommit(instance);
    }
  }
  // Otherwise, create our single instance (if necessary) and update
  // the transform (if necessary).
  else {
    bool newInstance = false;
    if (_ospInstances.size() == 0) {
      //convert aligned matrix to unalighned 4x3 matrix
      auto instance = ospNewInstance(instanceModel, (osp::affine3f&)ospcommon::one);
      _ospInstances.push_back(instance);
      ospCommit(instance);
      newInstance = true;
    }
    if (newInstance || HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
      //TODO: update transform
      auto instance = _ospInstances[0];
      float* xfm = _transform.GetArray();
      //convert aligned matrix to unalighned 4x3 matrix
      ospSet3f(instance,"xfm.l.vx",xfm[0],xfm[1],xfm[2]);
      ospSet3f(instance,"xfm.l.vy",xfm[4],xfm[5],xfm[6]);
      ospSet3f(instance,"xfm.l.vz",xfm[8],xfm[9],xfm[10]);
      ospSet3f(instance,"xfm.p",xfm[12],xfm[13],xfm[14]);
    }
    if (newInstance || newMesh ||
                    HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
                    HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
                                                    HdTokens->points)) {
      ospCommit(_ospInstances[0]);
      // Mark the instance as updated in the top-level BVH.
    }
  }

  // Update visibility by pulling the object into/out of the model.
  if (_sharedData.visible) {
    for (auto instance : _ospInstances) {
      //std::lock_guard<std::mutex> lock(g_mutex);
      ospAddGeometry(model, instance);
    }
  } else {
    //TODO: ospRemove geometry?
  }

  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}


void
HdOSPRayMesh::_UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                         HdStDrawItem *drawItem,
                                         const HdMeshReprDesc &desc,
                                         size_t drawItemIdForDesc)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    bool hasFaceVaryingPrimvars =
        (bool)drawItem->GetFaceVaryingPrimvarRange();


    HdSt_GeometricShader::PrimitiveType primType =
        HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;

    // resolve geom style, cull style
    HdCullStyle cullStyle = desc.cullStyle;
    HdMeshGeomStyle geomStyle = desc.geomStyle;

    // Resolve normals interpolation.
    HdInterpolation normalsInterpolation = HdInterpolationVertex;

    // if the repr doesn't have an opinion about cullstyle, use the
    // prim's default (it could also be DontCare, then renderPass's
    // cullStyle is going to be used).
    //
    // i.e.
    //   Repr CullStyle > Rprim CullStyle > RenderPass CullStyle
    //
    if (cullStyle == HdCullStyleDontCare) {
        cullStyle = _cullStyle;
    }

    bool blendWireframeColor = desc.blendWireframeColor;

    // Check if the shader bound to this mesh has a custom displacement
    // terminal, or uses ptex, so that we know whether to include the geometry
    // shader.
    const HdOSPRayMaterial *material = static_cast<const HdOSPRayMaterial *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));

    //    bool hasCustomDisplacementTerminal =
    //        material && material->HasDisplacement();
    bool hasPtex = material && material->HasPtex();

    // The edge geomstyles below are rasterized as lines.
    // See HdSt_GeometricShader::BindResources()
    // The batches need to be validated and rebuilt if necessary.
    renderIndex.GetChangeTracker().MarkBatchesDirty();
}

PXR_NAMESPACE_CLOSE_SCOPE
