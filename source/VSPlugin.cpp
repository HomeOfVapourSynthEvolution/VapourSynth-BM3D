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


static void VS_CC RGB2OPP_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    RGB2OPP_Data *d = reinterpret_cast<RGB2OPP_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.format = RGB2OPP_Process::NewFormat(*d, d->vi->format, core, vsapi);

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC RGB2OPP_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const RGB2OPP_Data *d = reinterpret_cast<RGB2OPP_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "RGB2OPP", RGB2OPP_Init, RGB2OPP_GetFrame, RGB2OPP_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.OPP2RGB


static void VS_CC OPP2RGB_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    OPP2RGB_Data *d = reinterpret_cast<OPP2RGB_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.format = OPP2RGB_Process::NewFormat(*d, d->vi->format, core, vsapi);

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC OPP2RGB_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const OPP2RGB_Data *d = reinterpret_cast<OPP2RGB_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "OPP2RGB", OPP2RGB_Init, OPP2RGB_GetFrame, OPP2RGB_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.Basic


static void VS_CC BM3D_Basic_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Basic_Data *d = reinterpret_cast<BM3D_Basic_Data *>(*instanceData);

    vsapi->setVideoInfo(d->vi, 1, node);
}

static const VSFrameRef *VS_CC BM3D_Basic_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const BM3D_Basic_Data *d = reinterpret_cast<BM3D_Basic_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "Basic", BM3D_Basic_Init, BM3D_Basic_GetFrame, BM3D_Basic_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.Final


static void VS_CC BM3D_Final_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    BM3D_Final_Data *d = reinterpret_cast<BM3D_Final_Data *>(*instanceData);

    vsapi->setVideoInfo(d->vi, 1, node);
}

static const VSFrameRef *VS_CC BM3D_Final_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const BM3D_Final_Data *d = reinterpret_cast<BM3D_Final_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "Final", BM3D_Final_Init, BM3D_Final_GetFrame, BM3D_Final_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VBasic


static void VS_CC VBM3D_Basic_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Basic_Data *d = reinterpret_cast<VBM3D_Basic_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.format = VBM3D_Basic_Process::NewFormat(*d, d->vi->format, core, vsapi);
    dvi.height = d->vi->height * (d->para.radius * 2 + 1) * 2;

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC VBM3D_Basic_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VBM3D_Basic_Data *d = reinterpret_cast<VBM3D_Basic_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "VBasic", VBM3D_Basic_Init, VBM3D_Basic_GetFrame, VBM3D_Basic_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VFinal


static void VS_CC VBM3D_Final_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    VBM3D_Final_Data *d = reinterpret_cast<VBM3D_Final_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.format = VBM3D_Final_Process::NewFormat(*d, d->vi->format, core, vsapi);
    dvi.height = d->vi->height * (d->para.radius * 2 + 1) * 2;

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC VBM3D_Final_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VBM3D_Final_Data *d = reinterpret_cast<VBM3D_Final_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "VFinal", VBM3D_Final_Init, VBM3D_Final_GetFrame, VBM3D_Final_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: bm3d.VAggregate


static void VS_CC VAggregate_Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    VAggregate_Data *d = reinterpret_cast<VAggregate_Data *>(*instanceData);

    VSVideoInfo dvi = *(d->vi);
    dvi.format = VAggregate_Process::NewFormat(*d, d->vi->format, core, vsapi);
    dvi.height = d->vi->height / (d->radius * 2 + 1) / 2;

    vsapi->setVideoInfo(&dvi, 1, node);
}

static const VSFrameRef *VS_CC VAggregate_GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const VAggregate_Data *d = reinterpret_cast<VAggregate_Data *>(*instanceData);

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

    // Create filter
    vsapi->createFilter(in, out, "VAggregate", VAggregate_Init, VAggregate_GetFrame, VAggregate_Free, fmParallel, 0, d, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VapourSynth: plugin initialization


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("com.vapoursynth.bm3d", "bm3d",
        "Implementation of BM3D denoising filter for VapourSynth.",
        VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("RGB2OPP",
        "input:clip;"
        "sample:int:opt;",
        RGB2OPP_Create, nullptr, plugin);

    registerFunc("OPP2RGB",
        "input:clip;"
        "sample:int:opt;",
        OPP2RGB_Create, nullptr, plugin);

    registerFunc("Basic",
        "input:clip;"
        "ref:clip:opt;"
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
        BM3D_Basic_Create, nullptr, plugin);

    registerFunc("Final",
        "input:clip;"
        "ref:clip;"
        "profile:data:opt;"
        "sigma:float[]:opt;"
        "block_size:int:opt;"
        "block_step:int:opt;"
        "group_size:int:opt;"
        "bm_range:int:opt;"
        "bm_step:int:opt;"
        "th_mse:float:opt;"
        "matrix:int:opt;",
        BM3D_Final_Create, nullptr, plugin);

    registerFunc("VBasic",
        "input:clip;"
        "ref:clip:opt;"
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
        VBM3D_Basic_Create, nullptr, plugin);

    registerFunc("VFinal",
        "input:clip;"
        "ref:clip;"
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
        VBM3D_Final_Create, nullptr, plugin);

    registerFunc("VAggregate",
        "input:clip;"
        "radius:int:opt;"
        "sample:int:opt;",
        VAggregate_Create, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
