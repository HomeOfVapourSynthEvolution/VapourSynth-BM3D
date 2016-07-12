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


#ifndef RGB2OPP_H_
#define RGB2OPP_H_


#include "Helper.h"
#include "Conversion.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class RGB2OPP_Data
    : public VSData
{
public:
    typedef RGB2OPP_Data _Myt;
    typedef VSData _Mybase;

public:
    VSSampleType sample;

public:
    RGB2OPP_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "RGB2OPP", std::string _NameSpace = "bm3d")
        : _Mybase(_vsapi, _FunctionName, _NameSpace)
    {}

    virtual int arguments_process(const VSMap *in, VSMap *out) override
    {
        int error;

        // input - clip
        node = vsapi->propGetNode(in, "input", 0, nullptr);
        vi = vsapi->getVideoInfo(node);

        if (!isConstantFormat(vi))
        {
            setError(out, "Invalid input clip, only constant format input supported");
            return 1;
        }
        if ((vi->format->sampleType == stInteger && vi->format->bitsPerSample > 16)
            || (vi->format->sampleType == stFloat && vi->format->bitsPerSample != 32))
        {
            setError(out, "Invalid input clip, only 8-16 bit int or 32 bit float formats supported");
            return 1;
        }
        if (vi->format->colorFamily != cmRGB)
        {
            setError(out, "Invalid input clip, must be of RGB color family");
            return 1;
        }

        // sample - int
        sample = static_cast<VSSampleType>(vsapi->propGetInt(in, "sample", 0, &error));

        if (error)
        {
            sample = stInteger;
        }
        else if (sample != stInteger && sample != stFloat)
        {
            setError(out, "Invalid \'sample\' assigned, must be 0 (integer sample type) or 1 (float sample type)");
            return 1;
        }

        return 0;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class RGB2OPP_Process
    : public VSProcess
{
public:
    typedef RGB2OPP_Process _Myt;
    typedef VSProcess _Mybase;
    typedef RGB2OPP_Data _Mydata;

private:
    const _Mydata &d;

private:
    template < typename _Dt1, typename _St1 >
    void process_core();

protected:
    virtual void process_core8() override
    {
        if (d.sample == 0)
        {
            process_core<uint16_t, uint8_t>();
        }
        else
        {
            process_core<FLType, uint8_t>();
        }
    }

    virtual void process_core16() override
    {
        if (d.sample == 0)
        {
            process_core<uint16_t, uint16_t>();
        }
        else
        {
            process_core<FLType, uint16_t>();
        }
    }

    virtual void process_coreS() override
    {
        if (d.sample == 0)
        {
            process_core<uint16_t, float>();
        }
        else
        {
            process_core<FLType, float>();
        }
    }

public:
    RGB2OPP_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {}

    // Always output YUV444P16 or YUV444PS
    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(cmYUV, d.sample, d.sample == 1 ? 32 : 16, 0, 0, core);
    }

protected:
    virtual void NewFormat() override
    {
        dfi = NewFormat(d, fi, core, vsapi);
    }

    virtual void NewFrame() override
    {
        _NewFrame(width, height, false);

        // Set output frame properties
        VSMap *dst_map = vsapi->getFramePropsRW(dst);

        vsapi->propSetInt(dst_map, "_Matrix", 2, paReplace);
        vsapi->propSetInt(dst_map, "BM3D_OPP", 1, paReplace);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class RGB2OPP_Process


template < typename _Dt1, typename _St1 >
void RGB2OPP_Process::process_core()
{
    // Get write/read pointer
    auto dstY = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 0));
    auto dstU = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 1));
    auto dstV = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 2));

    auto srcR = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 0));
    auto srcG = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 1));
    auto srcB = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 2));

    // Matrix conversion
    _Dt1 dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC;
    _St1 sFloor, sCeil;

    GetQuanPara(dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC, dfi->bitsPerSample, true);
    GetQuanPara(sFloor, sCeil, fi->bitsPerSample, true);

    MatrixConvert_RGB2YUV(dstY, dstU, dstV, srcR, srcG, srcB, height, width, dst_stride[0], src_stride[0],
        dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC, sFloor, sCeil, ColorMatrix::OPP, !isFloat(_Dt1));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
