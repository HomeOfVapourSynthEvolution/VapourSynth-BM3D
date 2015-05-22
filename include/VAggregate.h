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


#ifndef VAGGREGATE_H_
#define VAGGREGATE_H_


#include "BM3D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VAggregate_Data
    : public VSData
{
public:
    typedef VAggregate_Data _Myt;
    typedef VSData _Mybase;

public:
    int radius;
    VSSampleType sample;

public:
    VAggregate_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "VAggregate", std::string _NameSpace = "bm3d")
        : _Mybase(_vsapi, _FunctionName, _NameSpace)
    {}

    virtual int arguments_process(const VSMap *in, VSMap *out) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VAggregate_Process
    : public VSProcess
{
public:
    typedef VAggregate_Process _Myt;
    typedef VSProcess _Mybase;
    typedef VAggregate_Data _Mydata;

private:
    const _Mydata &d;

protected:
    int b_offset;
    int f_offset;
    int cur;
    int frames;

    std::vector<const VSFrameRef *> v_src;

    PCType src_height[VSMaxPlaneCount];
    PCType src_pcount[VSMaxPlaneCount];

    int process_plane[VSMaxPlaneCount];

    bool full = true;

private:
    template < typename _Dt1 >
    void process_core();

    template < typename _Dt1 >
    void process_core_gray();

    template < typename _Dt1 >
    void process_core_yuv();

protected:
    virtual void process_coreS() override
    {
        if (d.sample == 0)
        {
            process_core<uint16_t>();
        }
        else
        {
            process_core<FLType>();
        }
    }

public:
    VAggregate_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {
        int total_frames = d.vi->numFrames;
        int radius = d.radius;

        b_offset = -Min(n - 0, radius);
        f_offset = Min(total_frames - 1 - n, radius);
        cur = -b_offset;
        frames = f_offset - b_offset + 1;

        for (int o = b_offset; o <= f_offset; ++o)
        {
            if (o == 0)
            {
                v_src.push_back(src);
            }
            else
            {
                v_src.push_back(vsapi->getFrameFilter(n + o, d.node, frameCtx));
            }
        }
    }

    // Always output 16bit int or 32bit float Gray/YUV
    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(f->colorFamily, d.sample, d.sample == 1 ? 32 : 16,
            f->subSamplingW, f->subSamplingH, core);
    }

protected:
    virtual void NewFormat() override
    {
        dfi = NewFormat(d, fi, core, vsapi);
    }

    virtual void NewFrame() override
    {
        // Get input frame properties
        int error;
        const VSMap *src_map = vsapi->getFramePropsRO(src);

        int p_radius = int64ToIntS(vsapi->propGetInt(src_map, "BM3D_V_radius", 0, &error));

        if (error)
        {
            vsapi->setFilterError("bm3d.VAggregate - error: "
                "No frame property \"BM3D_V_radius\" exists in the input frame. "
                "Make sure you call bm3d.VAggregate next to bm3d.VBasic or bm3d.VFinal. ", frameCtx);
        }
        else if (d.radius != p_radius)
        {
            std::string msg, temp;
            std::stringstream ss;

            msg += "bm3d.VAggregate - error: Mismatch between argument \"radius=";
            msg += GetStr(d.radius);
            msg += "\" and the input frame property \"BM3D_V_radius=";
            msg += GetStr(p_radius);
            msg += "\" which indicates the radius used in previous filter (bm3d.VBasic or bm3d.VFinal).";

            vsapi->setFilterError(msg.c_str(), frameCtx);
        }

        int m = vsapi->propNumElements(src_map, "BM3D_V_process");
        const int64_t *p_process = vsapi->propGetIntArray(src_map, "BM3D_V_process", &error);

        if (error || m != VSMaxPlaneCount)
        {
            vsapi->setFilterError("bm3d.VAggregate - error: "
                "No valid frame property \"BM3D_V_process\" exists in the input frame. "
                "Make sure you call bm3d.VAggregate next to bm3d.VBasic or bm3d.VFinal. ", frameCtx);
        }
        else
        {
            for (int i = 0; i < VSMaxPlaneCount; i++)
            {
                process_plane[i] = int64ToIntS(p_process[i]);
            }
        }

        // Determine OPP input
        int64_t BM3D_OPP = vsapi->propGetInt(src_map, "BM3D_OPP", 0, &error);

        if (error)
        {
            BM3D_OPP = 0;
        }

        // Determine color range of Gray/YUV/YCoCg input
        int64_t _ColorRange = vsapi->propGetInt(src_map, "_ColorRange", 0, &error);

        if (error || BM3D_OPP == 1)
        {
            full = true;
        }
        else
        {
            full = _ColorRange != 1;
        }

        // The output frame is of original frame size (same as input frame of VBasic or VFinal)
        _NewFrame(width, height / (d.radius * 2 + 1) / 2, false);

        // Set the internal used src_height and src_pcount to a single part of the stacked data for convenience
        for (int i = 0; i < PlaneCount; ++i)
        {
            src_height[i] = dst_height[i];
            src_pcount[i] = src_height[i] * src_stride[i];
        }

        // Set output frame properties
        VSMap *dst_map = vsapi->getFramePropsRW(dst);

        vsapi->propDeleteKey(dst_map, "BM3D_V_radius");
        vsapi->propDeleteKey(dst_map, "BM3D_V_process");
    }

    void Kernel(FLType *dst, std::vector<const FLType *> ResNum, std::vector<const FLType *> ResDen);

    void Kernel(FLType *dstY, FLType *dstU, FLType *dstV,
        std::vector<const FLType *> ResNumY, std::vector<const FLType *> ResDenY,
        std::vector<const FLType *> ResNumU, std::vector<const FLType *> ResDenU,
        std::vector<const FLType *> ResNumV, std::vector<const FLType *> ResDenV);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "VAggregate.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
