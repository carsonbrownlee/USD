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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdOSPRay/mesh.h"

#include "pxr/imaging/hdOSPRay/config.h"
#include "pxr/imaging/hdOSPRay/context.h"
#include "pxr/imaging/hdOSPRay/instancer.h"
#include "pxr/imaging/hdOSPRay/renderParam.h"
#include "pxr/imaging/hdOSPRay/renderPass.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

std::mutex g_mutex;

HdOSPRayMesh::HdOSPRayMesh(SdfPath const& id,
                           SdfPath const& instancerId)
    : HdMesh(id, instancerId)
    , _adjacencyValid(false)
    , _normalsValid(false)
    , _refined(false)
    , _smoothNormals(false)
    , _doubleSided(false)
    , _cullStyle(HdCullStyleDontCare)
    , _ospMesh(nullptr)
{
}

void
HdOSPRayMesh::Finalize(HdRenderParam *renderParam)
{
    OSPModel scene = static_cast<HdOSPRayRenderParam*>(renderParam)
        ->GetOSPRayModel();
    // Delete any instances of this mesh in the top-level embree scene.
    _rtcInstanceIds.clear();
    //TODO: clean up ospray data

}

HdDirtyBits
HdOSPRayMesh::_GetInitialDirtyBits() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtMesh(), so it should list every data item
    // that _PopluateRtMesh requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyRefineLevel
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimVar
        | HdChangeTracker::DirtyNormals
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdOSPRayMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdOSPRayMesh::_InitRepr(TfToken const &reprName,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(reprName);
    TF_UNUSED(dirtyBits);

    // No-op
}


HdReprSharedPtr const &
HdOSPRayMesh::_GetRepr(HdSceneDelegate *sceneDelegate,
                       TfToken const &reprName,
                       HdDirtyBits *dirtyBits)
{
    TF_UNUSED(sceneDelegate);
    TF_UNUSED(reprName);
    TF_UNUSED(dirtyBits);

    // OSPRay doesn't use the HdRepr structure, so return an empty value.
    static HdReprSharedPtr empty;
    return empty;
}

void
HdOSPRayMesh::Sync(HdSceneDelegate* sceneDelegate,
                   HdRenderParam*   renderParam,
                   HdDirtyBits*     dirtyBits,
                   TfToken const&   reprName,
                   bool             forcedRepr)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The repr token is used to look up an HdMeshReprDesc struct, which
    // has drawing settings for this prim to use. Repr opinions can come
    // from the render pass's rprim collection or the scene delegate;
    // _GetReprName resolves these multiple opinions.
    TfToken calculatedReprName = _GetReprName(sceneDelegate, reprName,
        forcedRepr, dirtyBits);

    // XXX: Meshes can have multiple reprs; this is done, for example, when
    // the drawstyle specifies different rasterizing modes between front faces
    // and back faces. With raytracing, this concept makes less sense, but
    // combining semantics of two HdMeshReprDesc is tricky in the general case.
    // For now, HdOSPRayMesh only respects the first desc; this should be fixed.
    _MeshReprConfig::DescArray descs = _GetReprDesc(calculatedReprName);
    const HdMeshReprDesc &desc = descs[0];

    // Pull top-level embree state out of the render param.
//    RTCScene scene = static_cast<HdOSPRayRenderParam*>(renderParam)
//    ->GetOSPRayScene();
//    RTCDevice device = static_cast<HdOSPRayRenderParam*>(renderParam)
//        ->GetOSPRayDevice();
    OSPModel model = static_cast<HdOSPRayRenderParam*>(renderParam)->GetOSPRayModel();
    OSPRenderer renderer = static_cast<HdOSPRayRenderParam*>(renderParam)->GetOSPRayRenderer();

    // Create embree geometry objects.
    _PopulateRtMesh(sceneDelegate, model, renderer, dirtyBits, desc);
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
    // the points primvar is processed by _PopulateRtMesh. We only call
    // GetPrimVar on primvars that have been marked dirty.
    //
    // Currently, hydra doesn't have a good way of communicating changes in
    // the set of primvars, so we only ever add and update to the primvar set.

    TfTokenVector names = GetPrimVarVertexNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimVar(sceneDelegate, *nameIt),
                HdInterpolationVertex
            };
        }
    }
    names = GetPrimVarVaryingNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimVar(sceneDelegate, *nameIt),
                HdInterpolationVarying
            };
        }
    }
    names = GetPrimVarFacevaryingNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimVar(sceneDelegate, *nameIt),
                HdInterpolationFaceVarying
            };
        }
    }
    names = GetPrimVarUniformNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimVar(sceneDelegate, *nameIt),
                HdInterpolationUniform
            };
        }
    }
    names = GetPrimVarConstantNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimVar(sceneDelegate, *nameIt),
                HdInterpolationConstant
            };
        }
    }
}

void
HdOSPRayMesh::_PopulateRtMesh(HdSceneDelegate* sceneDelegate,
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

    if (HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->points)) {
        VtValue value = sceneDelegate->Get(id, HdTokens->points);
        _points = value.Get<VtVec3fArray>();
        //Carson: grabbing normals and colors appears to crash
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
    if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        _topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
    }
    if (HdChangeTracker::IsRefineLevelDirty(*dirtyBits, id)) {
        _topology = HdMeshTopology(_topology,
            sceneDelegate->GetRefineLevel(id));
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
    if (HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->widths) ||
        HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->primVar)) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    ////////////////////////////////////////////////////////////////////////
    // 2. Resolve drawstyles

    // The repr defines a set of geometry styles for drawing the mesh
    // (see hd/enums.h). We're ignoring points and wireframe for now, so
    // HdMeshGeomStyleSurf maps to subdivs and everything else maps to
    // HdMeshGeomStyleHull (coarse triangulated mesh).
    bool doRefine = (desc.geomStyle == HdMeshGeomStyleSurf);

    // If the subdivision scheme is "none", force us to not refine.
    doRefine = doRefine && _topology.GetScheme() != PxOsdOpenSubdivTokens->none;

    // The repr defines whether we should compute smooth normals for this mesh:
    // per-vertex normals taken as an average of adjacent faces, and
    // interpolated smoothly across faces.
    _smoothNormals = desc.smoothNormals;

    // If the subdivision scheme is "none" or "bilinear", force us not to use
    // smooth normals.
    _smoothNormals = _smoothNormals &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->none) &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->bilinear);

    // If the scene delegate has provided authored normals, force us to not use
    // smooth normals.
    bool authoredNormals = false;
    if (_primvarSourceMap.count(HdTokens->normals) > 0) {
        authoredNormals = true;
    }
    _smoothNormals = _smoothNormals && !authoredNormals;

    ////////////////////////////////////////////////////////////////////////
    // 3. Populate embree prototype object.

    // If the topology has changed, or the value of doRefine has changed, we
    // need to create or recreate the embree mesh object.
    // _GetInitialDirtyBits() ensures that the topology is dirty the first time
    // this function is called, so that the embree mesh is always created.
    bool newMesh = false;
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
        doRefine != _refined) {

        newMesh = true;

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

    // Update the smooth normals in steps:
    // 1. If the topology is dirty, update the adjacency table, a processed
    //    form of the topology that helps calculate smooth normals quickly.
    // 2. If the points are dirty, update the smooth normal buffer itself.
    if (_smoothNormals && !_adjacencyValid) {
        _adjacency.BuildAdjacencyTable(&_topology);
        _adjacencyValid = true;
        // If we rebuilt the adjacency table, force a rebuild of normals.
        _normalsValid = false;
    }
    if (_smoothNormals && !_normalsValid) {
        _computedNormals = _adjacency.ComputeSmoothNormals(_points.size(),
            _points.cdata());
        _normalsValid = true;
    }

    // Create new OSP Mesh
    if (newMesh || 
         HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->points)) {

      auto mesh = ospNewGeometry("trianglemesh");

      HdMeshUtil meshUtil(&_topology, GetId());
      meshUtil.ComputeTriangleIndices(&_triangulatedIndices,
                                      &_trianglePrimitiveParams);

      if (_primvarSourceMap.count(HdTokens->color) > 0) {
        auto& colorBuffer = _primvarSourceMap[HdTokens->color].data;
        if (colorBuffer.GetArraySize())
          _colors = colorBuffer.Get<VtVec4fArray>();
      }

      auto indices = ospNewData(_triangulatedIndices.size(), OSP_INT3,
        _triangulatedIndices.cdata(), OSP_DATA_SHARED_BUFFER);

      ospCommit(indices);
      ospSetData(mesh, "index", indices);

      auto vertices = ospNewData(_points.size(),OSP_FLOAT3, _points.cdata(),
        OSP_DATA_SHARED_BUFFER);
      ospCommit(vertices);
      ospSetData(mesh, "vertex", vertices);
      if (_computedNormals.size()) {
        auto normals = ospNewData(_computedNormals.size(),OSP_FLOAT3,
          _computedNormals.cdata(), OSP_DATA_SHARED_BUFFER);
        ospSetData(mesh, "vertex.normal", normals);
      }
      if (_colors.size() > 1) {
        //Carson: apparently colors are actually stored as a single color value for entire object
        auto colors = ospNewData(_colors.size(),OSP_FLOAT4, _colors.cdata(),
          OSP_DATA_SHARED_BUFFER);
        ospSetData(mesh, "vertex.color", colors);
      }

      OSPMaterial material;
      if (HdOSPRayConfig::GetInstance().usePathTracing == 1)
      {
        material = ospNewMaterial(renderer, "Principled");
        if (_colors.size() == 1)
        {
          ospSet3fv(material,"baseColor",static_cast<float*>(&_colors[0][0]));
          ospSet1f(material,"transmission",1.f-_colors[0][3]);
          ospSet1f(material,"roughness", 0.1f);
          ospSet1f(material,"specular", 0.1f);
          ospSet1f(material,"metallic", 0.f);
        }
      }
      else
      {
        material = ospNewMaterial(renderer, "OBJMaterial");
        //Carson: apparently colors are actually stored as a single color value for entire object
        ospSetf(material,"Ns",10.f);
        ospSet3f(material,"Ks",0.2f,0.2f,0.2f);
        if (_colors.size() == 1)
        {
          ospSet3fv(material,"Kd",static_cast<float*>(&_colors[0][0]));
          ospSet1f(material,"d",_colors[0][3]);
        }
      }

      ospCommit(material);
      ospSetMaterial(mesh, material);

      ospCommit(mesh);

      {
        std::lock_guard<std::mutex> lock(g_mutex);
        ospAddGeometry(model, mesh); // crashing when added to the scene. I suspect indices/vertex spec.
      }
    }

    // Update visibility by pulling the object into/out of the embree BVH.
//    if (_sharedData.visible) {
//        rtcEnable(_rtcMeshScene, _rtcMeshId);
//    } else {
//        rtcDisable(_rtcMeshScene, _rtcMeshId);
//    }

    ////////////////////////////////////////////////////////////////////////
    // 4. Populate embree instance objects.

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

        size_t oldSize = _rtcInstanceIds.size();
        size_t newSize = transforms.size();

        // Size down (if necessary).
        for(size_t i = newSize; i < oldSize; ++i) {
            // Delete instance context first...
//            delete _GetInstanceContext(scene, i);
            // Then OSPRay instance.
//            rtcDeleteGeometry(scene, _rtcInstanceIds[i]);
        }
        _rtcInstanceIds.resize(newSize);

        // Size up (if necessary).
        for(size_t i = oldSize; i < newSize; ++i) {
            // Create the new instance.
//            _rtcInstanceIds[i] = rtcNewInstance(scene, _rtcMeshScene);
//            // Create the instance context.
//            HdOSPRayInstanceContext *ctx = new HdOSPRayInstanceContext;
//            ctx->rootScene = _rtcMeshScene;
//            rtcSetUserData(scene, _rtcInstanceIds[i], ctx);
        }

        // Update transforms.
        for (size_t i = 0; i < transforms.size(); ++i) {
            // Combine the local transform and the instance transform.
            GfMatrix4f matf = _transform * GfMatrix4f(transforms[i]);
            // Update the transform in the BVH.
//            rtcSetTransform(scene, _rtcInstanceIds[i],
//                RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, matf.GetArray());
            // Update the transform in the instance context.
//            _GetInstanceContext(scene, i)->objectToWorldMatrix = matf;
            // Mark the instance as updated in the BVH.
//            rtcUpdate(scene, _rtcInstanceIds[i]);
        }
    }
    // Otherwise, create our single instance (if necessary) and update
    // the transform (if necessary).
    else {
        bool newInstance = false;
        if (_rtcInstanceIds.size() == 0) {
            // Create our single instance.
//            _rtcInstanceIds.push_back(rtcNewInstance(scene, _rtcMeshScene));
//            // Create the instance context.
//            HdOSPRayInstanceContext *ctx = new HdOSPRayInstanceContext;
//            ctx->rootScene = _rtcMeshScene;
//            rtcSetUserData(scene, _rtcInstanceIds[0], ctx);
            // Update the flag to force-set the transform.
            newInstance = true;
        }
        if (newInstance || HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
            // Update the transform in the BVH.
//            rtcSetTransform(scene, _rtcInstanceIds[0],
//                RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, _transform.GetArray());
            // Update the transform in the render context.
//            _GetInstanceContext(scene, 0)->objectToWorldMatrix = _transform;
        }
        if (newInstance || newMesh ||
            HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
            HdChangeTracker::IsPrimVarDirty(*dirtyBits, id,
                                            HdTokens->points)) {
            // Mark the instance as updated in the top-level BVH.
//            rtcUpdate(scene, _rtcInstanceIds[0]);
        }
    }

    // Clean all dirty bits.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
