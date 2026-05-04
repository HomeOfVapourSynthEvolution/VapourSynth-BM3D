/*
* BM3D denoising filter - VapourSynth plugin
* Copyright (c) 2015-2016 mawen1250
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/


#include "RGB2OPP.h"
#include "OPP2RGB.h"
#include "BM3D_Basic.h"
#include "BM3D_Final.h"
#include "VBM3D_Basic.h"
#include "VBM3D_Final.h"
#include "VAggregate.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.RGB2OPP


static const VSFrame *VS_CC RGB2OPP_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const RGB2OPP_Data *d = reinterpret_cast<RGB2OPP_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        RGB2OPP_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC RGB2OPP_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    RGB2OPP_Data *d = reinterpret_cast<RGB2OPP_Data *>(instanceData);

    delete d;
}


static void VS_CC RGB2OPP_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    RGB2OPP_Data *d = new RGB2OPP_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    VSVideoInfo dvi = *(d->vi);
    vsapi->queryVideoFormat(&dvi.format, cfYUV, d->sample, d->sample == 1 ? 32 : 16, 0, 0, core);

    VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };

    // Create filter
    vsapi->createVideoFilter(out, "RGB2OPP", &dvi, RGB2OPP_GetFrame, RGB2OPP_Free, fmParallel, deps, 1, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.OPP2RGB


static const VSFrame *VS_CC OPP2RGB_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const OPP2RGB_Data *d = reinterpret_cast<OPP2RGB_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        OPP2RGB_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC OPP2RGB_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    OPP2RGB_Data *d = reinterpret_cast<OPP2RGB_Data *>(instanceData);

    delete d;
}


static void VS_CC OPP2RGB_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    OPP2RGB_Data *d = new OPP2RGB_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    VSVideoInfo dvi = *(d->vi);
    vsapi->queryVideoFormat(&dvi.format, cfRGB, d->sample, d->sample == 1 ? 32 : 16, 0, 0, core);

    VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };

    // Create filter
    vsapi->createVideoFilter(out, "OPP2RGB", &dvi, OPP2RGB_GetFrame, OPP2RGB_Free, fmParallel, deps, 1, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.Basic


static const VSFrame *VS_CC BM3D_Basic_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Basic_Data *d = reinterpret_cast<BM3D_Basic_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
        if (d->rdef) vsapi->requestFrameFilter(n, d->rnode, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        BM3D_Basic_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC BM3D_Basic_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Basic_Data *d = reinterpret_cast<BM3D_Basic_Data *>(instanceData);

    delete d;
}


static void VS_CC BM3D_Basic_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Basic_Data *d = new BM3D_Basic_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    std::vector<VSFilterDependency> deps = { { d->node, rpStrictSpatial } };
    if (d->rdef)
        deps.push_back({ d->rnode, rpStrictSpatial });

    // Create filter
    vsapi->createVideoFilter(out, "Basic", d->vi, BM3D_Basic_GetFrame, BM3D_Basic_Free, fmParallel, deps.data(), deps.size(), d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.Final


static const VSFrame *VS_CC BM3D_Final_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Final_Data *d = reinterpret_cast<BM3D_Final_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
        if (d->rdef) vsapi->requestFrameFilter(n, d->rnode, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        BM3D_Final_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC BM3D_Final_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Final_Data *d = reinterpret_cast<BM3D_Final_Data *>(instanceData);

    delete d;
}


static void VS_CC BM3D_Final_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Final_Data *d = new BM3D_Final_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    std::vector<VSFilterDependency> deps = { { d->node, rpStrictSpatial } };
    if (d->rdef)
        deps.push_back({ d->rnode, rpStrictSpatial });

    // Create filter
    vsapi->createVideoFilter(out, "Final", d->vi, BM3D_Final_GetFrame, BM3D_Final_Free, fmParallel, deps.data(), deps.size(), d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VBasic


static const VSFrame *VS_CC VBM3D_Basic_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VBM3D_Basic_Data *d = reinterpret_cast<VBM3D_Basic_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        const int total_frames = d->vi->numFrames;
        const int radius = d->para.radius;
        const int b_offset = -Min(n - 0, radius);
        const int f_offset = Min(total_frames - 1 - n, radius);

        for (int o = b_offset; o <= f_offset; ++o)
        {
            vsapi->requestFrameFilter(n + o, d->node, frameCtx);
            if (d->rdef) vsapi->requestFrameFilter(n + o, d->rnode, frameCtx);
        }
    }
    else if (activationReason == arAllFramesReady)
    {
        VBM3D_Basic_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC VBM3D_Basic_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Basic_Data *d = reinterpret_cast<VBM3D_Basic_Data *>(instanceData);

    delete d;
}


static void VS_CC VBM3D_Basic_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Basic_Data *d = new VBM3D_Basic_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    VSVideoInfo dvi = *(d->vi);
    vsapi->queryVideoFormat(&dvi.format, d->vi->format.colorFamily == cfRGB ? cfYUV : d->vi->format.colorFamily, stFloat, 32, d->vi->format.subSamplingW,
        d->vi->format.subSamplingH, core);
    dvi.height = d->vi->height * (d->para.radius * 2 + 1) * 2;

    std::vector<VSFilterDependency> deps = { { d->node, rpGeneral } };
    if (d->rdef)
        deps.push_back({ d->rnode, rpGeneral });

    // Create filter
    vsapi->createVideoFilter(out, "VBasic", &dvi, VBM3D_Basic_GetFrame, VBM3D_Basic_Free, fmParallel, deps.data(), deps.size(), d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VFinal


static const VSFrame *VS_CC VBM3D_Final_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VBM3D_Final_Data *d = reinterpret_cast<VBM3D_Final_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        const int total_frames = d->vi->numFrames;
        const int radius = d->para.radius;
        const int b_offset = -Min(n - 0, radius);
        const int f_offset = Min(total_frames - 1 - n, radius);

        for (int o = b_offset; o <= f_offset; ++o)
        {
            vsapi->requestFrameFilter(n + o, d->node, frameCtx);
            if (d->rdef) vsapi->requestFrameFilter(n + o, d->rnode, frameCtx);
        }
    }
    else if (activationReason == arAllFramesReady)
    {
        VBM3D_Final_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC VBM3D_Final_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Final_Data *d = reinterpret_cast<VBM3D_Final_Data *>(instanceData);

    delete d;
}


static void VS_CC VBM3D_Final_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Final_Data *d = new VBM3D_Final_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    VSVideoInfo dvi = *(d->vi);
    vsapi->queryVideoFormat(&dvi.format, d->vi->format.colorFamily == cfRGB ? cfYUV : d->vi->format.colorFamily, stFloat, 32, d->vi->format.subSamplingW,
        d->vi->format.subSamplingH, core);
    dvi.height = d->vi->height * (d->para.radius * 2 + 1) * 2;

    std::vector<VSFilterDependency> deps = { { d->node, rpGeneral } };
    if (d->rdef)
        deps.push_back({ d->rnode, rpGeneral });

    // Create filter
    vsapi->createVideoFilter(out, "VFinal", &dvi, VBM3D_Final_GetFrame, VBM3D_Final_Free, fmParallel, deps.data(), deps.size(), d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VAggregate


static const VSFrame *VS_CC VAggregate_GetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VAggregate_Data *d = reinterpret_cast<VAggregate_Data *>(instanceData);

    if (activationReason == arInitial)
    {
        const int total_frames = d->vi->numFrames;
        const int radius = d->radius;
        const int b_offset = -Min(n - 0, radius);
        const int f_offset = Min(total_frames - 1 - n, radius);

        for (int o = b_offset; o <= f_offset; ++o)
        {
            vsapi->requestFrameFilter(n + o, d->node, frameCtx);
        }
    }
    else if (activationReason == arAllFramesReady)
    {
        VAggregate_Process p(*d, n, frameCtx, core, vsapi);

        return p.process();
    }

    return nullptr;
}

static void VS_CC VAggregate_Free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    VAggregate_Data *d = reinterpret_cast<VAggregate_Data *>(instanceData);

    delete d;
}


static void VS_CC VAggregate_Create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    VAggregate_Data *d = new VAggregate_Data(vsapi);

    if (d->arguments_process(in, out))
    {
        delete d;
        return;
    }

    VSVideoInfo dvi = *(d->vi);
    vsapi->queryVideoFormat(&dvi.format, d->vi->format.colorFamily, d->sample, d->sample == 1 ? 32 : 16, d->vi->format.subSamplingW, d->vi->format.subSamplingH,
        core);
    dvi.height = d->vi->height / (d->radius * 2 + 1) / 2;

    VSFilterDependency deps[] = { {d->node, rpGeneral} };

    // Create filter
    vsapi->createVideoFilter(out, "VAggregate", &dvi, VAggregate_GetFrame, VAggregate_Free, fmParallel, deps, 1, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: plugin initialization


VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi)
{
    vspapi->configPlugin("com.vapoursynth.bm3d", "bm3d",
        "Implementation of BM3D denoising filter for VapourSynth.",
        VS_MAKE_VERSION(10, 1), VAPOURSYNTH_API_VERSION, 0, plugin);

    vspapi->registerFunction("RGB2OPP",
        "input:vnode;"
        "sample:int:opt;",
        "clip:vnode;",
        RGB2OPP_Create, nullptr, plugin);

    vspapi->registerFunction("OPP2RGB",
        "input:vnode;"
        "sample:int:opt;",
        "clip:vnode;",
        OPP2RGB_Create, nullptr, plugin);

    vspapi->registerFunction("Basic",
        "input:vnode;"
        "ref:vnode:opt;"
        "profile:data:opt;"
        "sigma:float[]:opt;"
        "block_size:int:opt;"
        "block_step:int:opt;"
        "group_size:int:opt;"
        "bm_range:int:opt;"
        "bm_step:int:opt;"
        "th_mse:float:opt;"
        "hard_thr:float:opt;"
        "matrix:int:opt;",
        "clip:vnode;",
        BM3D_Basic_Create, nullptr, plugin);

    vspapi->registerFunction("Final",
        "input:vnode;"
        "ref:vnode;"
        "profile:data:opt;"
        "sigma:float[]:opt;"
        "block_size:int:opt;"
        "block_step:int:opt;"
        "group_size:int:opt;"
        "bm_range:int:opt;"
        "bm_step:int:opt;"
        "th_mse:float:opt;"
        "matrix:int:opt;",
        "clip:vnode;",
        BM3D_Final_Create, nullptr, plugin);

    vspapi->registerFunction("VBasic",
        "input:vnode;"
        "ref:vnode:opt;"
        "profile:data:opt;"
        "sigma:float[]:opt;"
        "radius:int:opt;"
        "block_size:int:opt;"
        "block_step:int:opt;"
        "group_size:int:opt;"
        "bm_range:int:opt;"
        "bm_step:int:opt;"
        "ps_num:int:opt;"
        "ps_range:int:opt;"
        "ps_step:int:opt;"
        "th_mse:float:opt;"
        "hard_thr:float:opt;"
        "matrix:int:opt;",
        "clip:vnode;",
        VBM3D_Basic_Create, nullptr, plugin);

    vspapi->registerFunction("VFinal",
        "input:vnode;"
        "ref:vnode;"
        "profile:data:opt;"
        "sigma:float[]:opt;"
        "radius:int:opt;"
        "block_size:int:opt;"
        "block_step:int:opt;"
        "group_size:int:opt;"
        "bm_range:int:opt;"
        "bm_step:int:opt;"
        "ps_num:int:opt;"
        "ps_range:int:opt;"
        "ps_step:int:opt;"
        "th_mse:float:opt;"
        "matrix:int:opt;",
        "clip:vnode;",
        VBM3D_Final_Create, nullptr, plugin);

    vspapi->registerFunction("VAggregate",
        "input:vnode;"
        "radius:int:opt;"
        "sample:int:opt;",
        "clip:vnode;",
        VAggregate_Create, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
