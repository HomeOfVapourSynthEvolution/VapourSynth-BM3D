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


#ifndef OPP2RGB_H_
#define OPP2RGB_H_


#include "Helper.h"
#include "Conversion.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class OPP2RGB_Data
    : public VSData
{
public:
    typedef OPP2RGB_Data _Myt;
    typedef VSData _Mybase;

public:
    VSSampleType sample;

public:
    OPP2RGB_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "OPP2RGB", std::string _NameSpace = "bm3d")
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
        if (vi->format->sampleType == stFloat && vi->format->bitsPerSample != 32)
        {
            setError(out, "Invalid input clip, only 8-16 bit int or 32 bit float formats supported");
            return 1;
        }
        if (vi->format->colorFamily != cmYUV)
        {
            setError(out, "Invalid input clip, must be of YUV color family");
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


class OPP2RGB_Process
    : public VSProcess
{
public:
    typedef OPP2RGB_Process _Myt;
    typedef VSProcess _Mybase;
    typedef OPP2RGB_Data _Mydata;

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
    OPP2RGB_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {}

    // Always output RGB48 or RGBS
    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(cmRGB, d.sample, d.sample == 1 ? 32 : 16, 0, 0, core);
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

        vsapi->propDeleteKey(dst_map, "BM3D_OPP");
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class OPP2RGB_Process


template < typename _Dt1, typename _St1 >
void OPP2RGB_Process::process_core()
{
    // Get write/read pointer
    auto dstR = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 0));
    auto dstG = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 1));
    auto dstB = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 2));

    auto srcY = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 0));
    auto srcU = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 1));
    auto srcV = reinterpret_cast<const _St1 *>(vsapi->getReadPtr(src, 2));

    // Matrix conversion
    _Dt1 dFloor, dCeil;
    _St1 sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC;

    GetQuanPara(dFloor, dCeil, dfi->bitsPerSample, true);
    GetQuanPara(sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC, fi->bitsPerSample, true);

    MatrixConvert_YUV2RGB(dstR, dstG, dstB, srcY, srcU, srcV, height, width, dst_stride[0], src_stride[0],
        dFloor, dCeil, sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC, ColorMatrix::OPP, !isFloat(_Dt1));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
