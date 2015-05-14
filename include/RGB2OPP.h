/*
* BM3D denoising filter - VapourSynth plugin
* Copyright (C) 2015  mawen1250
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

    virtual int arguments_process(const VSMap *in, VSMap *out)
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
        if (vi->format->sampleType == stFloat && vi->format->bitsPerSample != 32)
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
    RGB2OPP_Process(const _Mydata &_d, int n, VSFrameContext *frameCtx, VSCore *core, const VSAPI *_vsapi)
        : _Mybase(_d, n, frameCtx, core, _vsapi), d(_d)
    {}

    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(cmYUV, d.sample, d.sample == 1 ? 32 : 16, 0, 0, core);
    }

protected:
    virtual void NewFormat() override
    {
        dfi = NewFormat(d, fi, core, vsapi);
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
        dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC, sFloor, sCeil, ColorMatrix::OPP);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
