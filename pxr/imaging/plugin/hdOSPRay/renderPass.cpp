//
// Copyright 2016 Pixar
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

#include "pxr/imaging/hdOSPRay/renderPass.h"

#include "pxr/imaging/hdOSPRay/config.h"
#include "pxr/imaging/hdOSPRay/context.h"
#include "pxr/imaging/hdOSPRay/mesh.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

HdOSPRayRenderPass::HdOSPRayRenderPass(HdRenderIndex *index,
                                       HdRprimCollection const &collection,
                                       OSPModel model, OSPRenderer renderer)
    : HdRenderPass(index, collection)
    , _pendingResetImage(false)
    , _pendingModelUpdate(true)
    , _width(0)
    , _height(0)
    , _model(model)
    , _renderer(renderer)
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _clearColor(0.0707f, 0.0707f, 0.0707f)
{
    _camera = ospNewCamera("perspective");
    std::vector<OSPLight> lights;
    auto ambient = ospNewLight(_renderer, "ambient");
    ospSet3f(ambient, "color", 1.f,1.f,1.f);
    ospSet1f(ambient,"intensity",0.6f);
    ospCommit(ambient);
    lights.push_back(ambient);
    auto sun = ospNewLight(_renderer, "DirectionalLight");
    ospSet3f(sun, "color", 1.f,232.f/255.f,166.f/255.f);
    ospSet3f(sun, "direction", 0.562f,0.25f,-0.25f);
    ospSet1f(sun,"intensity",4.3f);
    ospCommit(sun);
    lights.push_back(sun);
    auto bounce = ospNewLight(_renderer, "DirectionalLight");
    ospSet3f(bounce, "color", 127.f/255.f,178.f/255.f,255.f/255.f);
    ospSet3f(bounce, "direction", -0.13f,.94f,-.105f);
    ospSet1f(bounce,"intensity",0.35f);
    ospCommit(bounce);
    lights.push_back(bounce);

    OSPData lightArray = ospNewData(lights.size(), OSP_OBJECT,&(lights[0]));
    ospSetData(_renderer, "lights", lightArray);
    ospSet4f(_renderer,"bgColor",_clearColor[0],_clearColor[1],_clearColor[2],1.f);

    ospSetObject(_renderer, "model", _model);
    ospSetObject(_renderer, "camera", _camera);

    ospSet1i(_renderer,"spp",HdOSPRayConfig::GetInstance().samplesPerFrame);
    ospSet1i(_renderer,"aoSamples",HdOSPRayConfig::GetInstance().ambientOcclusionSamples);
    ospSet1i(_renderer,"maxDepth",5);
    ospSet1f(_renderer,"aoDistance",15.0f);
    ospSet1i(_renderer,"shadowsEnabled",true);
    ospSet1f(_renderer,"maxContribution",2.f);
    ospSet1f(_renderer,"minContribution",0.1f);
    ospSet1f(_renderer,"epsilon",0.0001f);

    ospCommit(_renderer);
}

HdOSPRayRenderPass::~HdOSPRayRenderPass()
{
}

void
HdOSPRayRenderPass::ResetImage()
{
    // Set a flag to clear the sample buffer the next time Execute() is called.
    _pendingResetImage = true;
}

bool
HdOSPRayRenderPass::IsConverged() const
{
    // A super simple heuristic: consider ourselves converged after N
    // samples. Since we currently uniformly sample the framebuffer, we can
    // use the sample count from pixel(0,0).
//    unsigned int samplesToConvergence =
//        HdOSPRayConfig::GetInstance().samplesToConvergence;
//    return (_sampleBuffer[3] >= samplesToConvergence);
  return false;
}

void
HdOSPRayRenderPass::_MarkCollectionDirty()
{
    // If the drawable collection changes, we should reset the sample buffer.
    _pendingResetImage = true;
    _pendingModelUpdate = true;
}

void
HdOSPRayRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const &renderTags)
{
    // XXX: Add collection and renderTags support.
    // XXX: Add clip planes support.

    // If the viewport has changed, resize the sample buffer.
    GfVec4f vp = renderPassState->GetViewport();
    if (_width != vp[2] || _height != vp[3]) {
        _width = vp[2];
        _height = vp[3];
        _frameBuffer = ospNewFrameBuffer(osp::vec2i({int(_width),int(_height)}),OSP_FB_RGBA8,OSP_FB_COLOR|OSP_FB_ACCUM);
        ospCommit(_frameBuffer);
        _colorBuffer.resize(_width*_height*4);
        _pendingResetImage = true;
    }

    // Reset the sample buffer if it's been requested.
    if (_pendingResetImage) {
      ospFrameBufferClear(_frameBuffer, OSP_FB_ACCUM);
      _pendingResetImage = false;
    }
    if (_pendingModelUpdate)
    {
      ospCommit(_model);
      _pendingModelUpdate = false;
    }

    // Update camera
    _inverseViewMatrix = renderPassState->GetWorldToViewMatrix().GetInverse();
    _inverseProjMatrix = renderPassState->GetProjectionMatrix().GetInverse();
    float aspect = _width/float(_height);
    ospSetf(_camera, "aspect", aspect);
    //ospSetf(_camera, "aspect", 2.86f); //TODO DEBUG:!!!! wtf is going on?  aspect ratio not updating in ospray
    // even though it's set to correct value unless explicitly set to a single value...
    std::cout << "aspect: " << aspect << std::endl;
    //std::cout << "width: " << _width << " height: " << _height << std::endl;
    GfVec3f origin = GfVec3f(0,0,0);
    GfVec3f dir = GfVec3f(0,0,-1);
    GfVec3f up = GfVec3f(0,1,0);
    origin = _inverseViewMatrix.Transform(origin);
    dir = _inverseViewMatrix.TransformDir(dir).GetNormalized();
    ospSet3fv(_camera,"pos", &origin[0]);
    ospSet3fv(_camera,"dir", &dir[0]);
    ospSet3fv(_camera,"up", &up[0]);
    ospSetf(_camera,"fovy", 60.f);
    ospCommit(_camera);

    //Render the frame
    ospRenderFrame(_frameBuffer,_renderer,OSP_FB_COLOR | OSP_FB_ACCUM);

    // Resolve the image buffer: find the average color per pixel by
    // dividing the summed color by the number of samples;
    // and convert the image into a GL-compatible format.
    const void* rgba = ospMapFrameBuffer(_frameBuffer, OSP_FB_COLOR);
    memcpy((void*)&_colorBuffer[0], rgba, _width*_height*4);
    ospUnmapFrameBuffer(rgba, _frameBuffer);

    // Blit!
    glDrawPixels(_width, _height, GL_RGBA, GL_UNSIGNED_BYTE, &_colorBuffer[0]);
}

PXR_NAMESPACE_CLOSE_SCOPE
