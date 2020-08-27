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


#ifndef VBM3D_FINAL_H_
#define VBM3D_FINAL_H_


#include "VBM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VBM3D_Final_Data
    : public VBM3D_Data_Base
{
public:
    typedef VBM3D_Final_Data _Myt;
    typedef VBM3D_Data_Base _Mybase;

public:
    VBM3D_Final_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "VFinal", std::string _NameSpace = "bm3d")
        : _Mybase(true, _vsapi, _FunctionName, _NameSpace)
    {}

    VBM3D_Final_Data(const _Myt &right) = delete;
    VBM3D_Final_Data(_Myt &&right) = delete;
    _Myt &operator=(const _Myt &right) = delete;
    _Myt &operator=(_Myt &&right) = delete;

    virtual ~VBM3D_Final_Data() override {
        if (wdef && wnode) vsapi->freeNode(wnode);
    }

    virtual int arguments_process(const VSMap *in, VSMap *out) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VBM3D_Final_Process
    : public VBM3D_Process_Base
{
public:
    typedef VBM3D_Final_Process _Myt;
    typedef VBM3D_Process_Base _Mybase;
    typedef VBM3D_Final_Data _Mydata;

private:
    const _Mydata &d;

public:
    VBM3D_Final_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d) {
        if (d.wdef) {
            for (int o = b_offset; o <= f_offset; ++o)
                v_wref.push_back(vsapi->getFrameFilter(n + o, d.wnode, frameCtx));

            wfi = vsapi->getFrameFormat(v_wref[cur]);
        }
        else {
            v_wref = v_ref;
            wfi = rfi;
        }

        if (!skip)
            for (int i = 0; i < PlaneCount; ++i) {
                wref_height[i] = vsapi->getFrameHeight(v_wref[cur], i);
                wref_width[i] = vsapi->getFrameWidth(v_wref[cur], i);
                wref_stride[i] = vsapi->getStride(v_wref[cur], i) / wfi->bytesPerSample;
                wref_pcount[i] = wref_height[i] * wref_stride[i];
            }
    }

    virtual ~VBM3D_Final_Process() override {
        if (d.wdef)
            for (int i = 0; i < frames; ++i)
                vsapi->freeFrame(v_wref[i]);
    }

protected:
    virtual void CollaborativeFilter(int plane,
        const std::vector<FLType *> &ResNum, const std::vector<FLType *> &ResDen,
        const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref,
        const Pos3PairCode &code) const override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
