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


#ifndef HELPER_H_
#define HELPER_H_


#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <vapoursynth/VapourSynth.h>
#include <vapoursynth/VSHelper.h>
#include "Type.h"
#include "Specification.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exception handle


#ifdef _DEBUG
#define DEBUG_BREAK __debugbreak();
#define DEBUG_FAIL(mesg) __debugbreak(); _STD _DEBUG_ERROR(mesg);
#else
#define DEBUG_BREAK exit(EXIT_FAILURE);
#define DEBUG_FAIL(mesg) std::cerr << mesg << std::endl; exit(EXIT_FAILURE);
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convert to std::string


template < typename _Ty >
std::string GetStr(const _Ty &src)
{
    std::stringstream ss;
    ss << src;
    return ss.str();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory allocation


const size_t MEMORY_ALIGNMENT = 32;


template < typename _Ty >
void AlignedMalloc(_Ty *&Memory, size_t Count, size_t Alignment = MEMORY_ALIGNMENT)
{
    Memory = vs_aligned_malloc<_Ty>(sizeof(_Ty) * Count, Alignment);
}


template < typename _Ty >
void AlignedFree(_Ty *&Memory)
{
    vs_aligned_free(Memory);
    Memory = nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2D array copy


template < typename _Dt1, typename _St1 >
void MatCopy(_Dt1 *dstp, const _St1 *srcp, PCType height, PCType width, PCType dst_stride, PCType src_stride)
{
    for (PCType j = 0; j < height; ++j)
    {
        for (PCType i = 0; i < width; ++i)
        {
            dstp[i] = static_cast<_Dt1>(srcp[i]);
        }

        dstp += dst_stride;
        srcp += src_stride;
    }
}

template < typename _Ty >
void MatCopy(_Ty *dstp, const _Ty *srcp, PCType height, PCType width, PCType dst_stride, PCType src_stride)
{
    if (height > 0)
    {
        if (src_stride == dst_stride && src_stride == width)
        {
            memcpy(dstp, srcp, sizeof(_Ty) * height * width);
        }
        else
        {
            for (PCType j = 0; j < height; ++j)
            {
                memcpy(dstp, srcp, sizeof(_Ty) * width);
                dstp += dst_stride;
                srcp += src_stride;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loop in 2D array


#define LOOP_V _Loop_V
#define LOOP_H _Loop_H
#define LOOP_Hinv _Loop_Hinv
#define LOOP_VH _Loop_VH


template < typename _Fn1 >
void _Loop_V(const PCType height, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        _Func(j);
    }
}

template < typename _Fn1 >
void _Loop_H(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType offset = 0;
    const PCType range = width;

    for (PCType j = 0; j < height; ++j)
    {
        const PCType lower = j * stride + offset;
        const PCType upper = lower + range;

        _Func(j, lower, upper);
    }
}

template < typename _Fn1 >
void _Loop_Hinv(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType offset = 0;
    const PCType range = width;

    for (PCType j = height - 1; j >= 0; --j)
    {
        const PCType lower = j * stride + offset;
        const PCType upper = lower + range;

        _Func(j, lower, upper);
    }
}

template < typename _Fn1 >
void _Loop_VH(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        PCType i = j * stride;

        for (const PCType upper = i + width; i < upper; ++i)
        {
            _Func(i);
        }
    }
}

template < typename _Fn1 >
void _Loop_VH(const PCType height, const PCType width, const PCType stride0, const PCType stride1, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        PCType i0 = j * stride0;
        PCType i1 = j * stride1;

        for (const PCType upper = i0 + width; i0 < upper; ++i0, ++i1)
        {
            _Func(i0, i1);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantization parameters


template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full, const std::false_type &)
{
    if (full)
    {
        Floor = static_cast<_Ty>(0);
        Ceil = static_cast<_Ty>((1 << bps) - 1);
    }
    else
    {
        Floor = static_cast<_Ty>(16 << (bps - 8));
        Ceil = static_cast<_Ty>(235 << (bps - 8));
    }
}

template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full, const std::true_type &)
{
    Floor = static_cast<_Ty>(0);
    Ceil = static_cast<_Ty>(1);
}

template < typename _Ty >
void GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full)
{
    _GetQuanPara(Floor, Ceil, bps, full, _IsFloat<_Ty>());
}


template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma, const std::false_type &)
{
    if (full)
    {
        Floor = static_cast<_Ty>(0);
        Neutral = chroma ? static_cast<_Ty>(1 << (bps - 1)) : Floor;
        Ceil = static_cast<_Ty>((1 << bps) - 1);
    }
    else
    {
        Floor = static_cast<_Ty>(16 << (bps - 8));
        Neutral = chroma ? static_cast<_Ty>(1 << (bps - 1)) : Floor;
        Ceil = static_cast<_Ty>(chroma ? 240 << (bps - 8) : 235 << (bps - 8));
    }
}

template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma, const std::true_type &)
{
    Floor = static_cast<_Ty>(chroma ? -0.5 : 0);
    Neutral = chroma ? static_cast<_Ty>(0) : Floor;
    Ceil = static_cast<_Ty>(chroma ? 0.5 : 1);
}

template < typename _Ty >
void GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma)
{
    _GetQuanPara(Floor, Neutral, Ceil, bps, full, chroma, _IsFloat<_Ty>());
}


template < typename _Ty >
void GetQuanPara(_Ty &FloorY, _Ty &CeilY, _Ty &FloorC, _Ty &NeutralC, _Ty &CeilC, int bps, bool full)
{
    GetQuanPara(FloorY, CeilY, bps, full);
    GetQuanPara(FloorC, NeutralC, CeilC, bps, full, true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
bool isChroma(_Ty Floor, _Ty Neutral)
{
    return Floor < Neutral;
}

template < typename _Ty >
bool _IsPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil, const std::false_type &)
{
    return Floor < Neutral && (Floor + Ceil) % 2 == 1;
}
template < typename _Ty >
bool _IsPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil, const std::true_type &)
{
    return false;
}
template < typename _Ty >
bool isPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil)
{
    return _IsPCChroma(Floor, Neutral, Ceil, _IsFloat<_Ty>());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
_Ty Min(const _Ty &a, const _Ty &b)
{
    return b < a ? b : a;
}

template < typename _Ty >
_Ty Max(const _Ty &a, const _Ty &b)
{
    return b > a ? b : a;
}

template < typename _Ty >
_Ty Clip(const _Ty &input, const _Ty &lower, const _Ty &upper)
{
    return input <= lower ? lower : input >= upper ? upper : input;
}


template < typename _Ty >
_Ty Abs(const _Ty &input)
{
    return input < 0 ? -input : input;
}

template < typename _Ty >
_Ty AbsSub(const _Ty &a, const _Ty &b)
{
    return b < a ? a - b : b - a;
}


template < typename _Ty >
_Ty _RoundDiv(_Ty dividend, _Ty divisor, const std::false_type &)
{
    return (dividend + divisor / 2) / divisor;
}

template < typename _Ty >
_Ty _RoundDiv(_Ty dividend, _Ty divisor, const std::true_type &)
{
    return dividend / divisor;
}

template < typename _Ty >
_Ty RoundDiv(_Ty dividend, _Ty divisor)
{
    return _RoundDiv(dividend, divisor, _IsFloat<_Ty>);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
int stride_cal(int width)
{
    size_t Alignment2 = MEMORY_ALIGNMENT / sizeof(_Ty);
    return static_cast<int>(width % Alignment2 == 0 ? width : (width / Alignment2 + 1) * Alignment2);
}


template < typename _Ty >
void data2buff(_Ty *dst, const _Ty *src, int xoffset, int yoffset,
    int bufheight, int bufwidth, int bufstride, int height, int width, int stride)
{
    int x, y;
    _Ty *dstp;
    const _Ty *srcp;

    for (y = 0; y < height; ++y)
    {
        dstp = dst + (yoffset + y) * bufstride;
        srcp = src + y * stride;
        for (x = 0; x < xoffset; ++x)
            dstp[x] = srcp[0];
        memcpy(dstp + xoffset, srcp, sizeof(_Ty) * width);
        for (x = xoffset + width; x < bufwidth; ++x)
            dstp[x] = srcp[width - 1];
    }

    srcp = dst + yoffset * bufstride;
    for (y = 0; y < yoffset; ++y)
    {
        dstp = dst + y * bufstride;
        memcpy(dstp, srcp, sizeof(_Ty) * bufwidth);
    }

    srcp = dst + (yoffset + height - 1) * bufstride;
    for (y = yoffset + height; y < bufheight; ++y)
    {
        dstp = dst + y * bufstride;
        memcpy(dstp, srcp, sizeof(_Ty) * bufwidth);
    }
}

template < typename _Ty >
_Ty *newbuff(const _Ty *src, int xoffset, int yoffset,
    int bufheight, int bufwidth, int bufstride, int height, int width, int stride)
{
    _Ty *dst;
    AlignedMalloc(dst, bufheight * bufstride);
    data2buff(dst, src, xoffset, yoffset, bufheight, bufwidth, bufstride, height, width, stride);
    return dst;
}

template < typename _Ty >
void freebuff(_Ty *&buff)
{
    AlignedFree(buff);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const int VSMaxPlaneCount = 3;


class VSData
{
public:
    typedef VSData _Myt;

protected:
    std::string NameSpace = "";
    std::string FunctionName = "";

public:
    const VSAPI *vsapi = nullptr;
    VSNodeRef *node = nullptr;
    const VSVideoInfo *vi = nullptr;

    int process[VSMaxPlaneCount];

protected:
    void setError(VSMap *out, const char *error_msg) const
    {
        std::string str = NameSpace + "." + FunctionName + ": " + error_msg;
        vsapi->setError(out, str.c_str());
    }

public:
    VSData(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "", std::string _NameSpace = "")
        : NameSpace(_NameSpace), FunctionName(_FunctionName), vsapi(_vsapi)
    {
        for (int i = 0; i < VSMaxPlaneCount; ++i)
        {
            process[i] = 1;
        }
    }

    VSData(const _Myt &right) = delete;

    VSData(_Myt &&right)
        : vsapi(right.vsapi), node(right.node), vi(right.vi)
    {
        for (int i = 0; i < VSMaxPlaneCount; ++i)
        {
            process[i] = right.process[i];
        }

        right.vsapi = nullptr;
        right.node = nullptr;
        right.vi = nullptr;
    }

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        if (node) vsapi->freeNode(node);

        vsapi = right.vsapi;
        node = right.node;
        vi = right.vi;

        for (int i = 0; i < VSMaxPlaneCount; ++i)
        {
            process[i] = right.process[i];
        }

        right.vsapi = nullptr;
        right.node = nullptr;
        right.vi = nullptr;

        return *this;
    }

    virtual ~VSData()
    {
        if (node) vsapi->freeNode(node);
    }

    virtual int arguments_process(const VSMap *in, VSMap *out) = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VSProcess
{
public:
    typedef VSProcess _Myt;
    typedef VSData _Mydata;

private:
    const _Mydata &d;

protected:
    int n = 0;
    VSFrameContext *frameCtx = nullptr;
    VSCore *core = nullptr;
    const VSAPI *vsapi = nullptr;

    const VSFrameRef *src = nullptr;
    const VSFormat *fi = nullptr;
    VSFrameRef *dst = nullptr;
    const VSFormat *dfi = nullptr;

    bool skip = true;

    int PlaneCount;
    int Bps; // Bytes per sample
    int bps; // bits per sample
    int flt; // 0 - integer, 1 - half precision float, 2 - single precision float, 3 - double precision float

    PCType height;
    PCType width;
    PCType stride;
    PCType pcount;

    PCType src_height[VSMaxPlaneCount];
    PCType src_width[VSMaxPlaneCount];
    PCType src_stride[VSMaxPlaneCount];
    PCType src_pcount[VSMaxPlaneCount];

    PCType dst_height[VSMaxPlaneCount];
    PCType dst_width[VSMaxPlaneCount];
    PCType dst_stride[VSMaxPlaneCount];
    PCType dst_pcount[VSMaxPlaneCount];

private:
    template < typename _Ty >
    void process_core();

    template < typename _Ty >
    void process_core_gray();

    template < typename _Ty >
    void process_core_yuv();

    template < typename _Ty >
    void process_core_rgb();

protected:
    virtual void process_core8() {};
    virtual void process_core16() {};
    virtual void process_coreH() {}
    virtual void process_coreS() {}

public:
    VSProcess(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : d(_d), n(_n), frameCtx(_frameCtx), core(_core), vsapi(_vsapi)
    {
        src = vsapi->getFrameFilter(n, d.node, frameCtx);
        fi = vsapi->getFrameFormat(src);

        PlaneCount = fi->numPlanes;
        Bps = fi->bytesPerSample;
        bps = fi->bitsPerSample;

        if (fi->sampleType == stFloat)
        {
            if (bps == 16) flt = 1;
            if (bps == 32) flt = 2;
            if (bps == 64) flt = 3;
        }
        else
        {
            flt = 0;
        }

        for (int i = 0; i < PlaneCount; ++i)
        {
            if (d.process[i]) skip = false;
        }

        if (!skip)
        {
            height = vsapi->getFrameHeight(src, 0);
            width = vsapi->getFrameWidth(src, 0);
            stride = vsapi->getStride(src, 0) / Bps;
            pcount = height * stride;

            for (int i = 0; i < PlaneCount; ++i)
            {
                src_height[i] = vsapi->getFrameHeight(src, i);
                src_width[i] = vsapi->getFrameWidth(src, i);
                src_stride[i] = vsapi->getStride(src, i) / fi->bytesPerSample;
                src_pcount[i] = src_height[i] * src_stride[i];
            }
        }
    }

    virtual ~VSProcess()
    {
        vsapi->freeFrame(src);
    }

    const VSFrameRef *process()
    {
        if (skip)
        {
            return src;
        }
        else
        {
            NewFormat();
            NewFrame();
        }

        if (flt == 1)
        {
            process_coreH();
        }
        else if (flt == 2)
        {
            process_coreS();
        }
        else if (Bps == 1)
        {
            process_core8();
        }
        else if (Bps == 2)
        {
            process_core16();
        }

        return dst;
    }

    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(f->colorFamily, f->sampleType, f->bitsPerSample,
            f->subSamplingW, f->subSamplingH, core);
    }

protected:
    virtual void NewFormat()
    {
        dfi = NewFormat(d, fi, core, vsapi);
    }

    virtual void NewFrame()
    {
        _NewFrame(width, height, dfi == fi);
    }

    void _NewFrame(PCType _width, PCType _height, bool copy = true)
    {
        if (!skip)
        {
            if (copy)
            {
                int planes[VSMaxPlaneCount];
                const VSFrameRef *cp_planes[VSMaxPlaneCount];

                for (int i = 0; i < VSMaxPlaneCount; ++i)
                {
                    planes[i] = i;
                    cp_planes[i] = d.process[i] ? nullptr : src;
                }

                dst = vsapi->newVideoFrame2(dfi, _width, _height, cp_planes, planes, src, core);
            }
            else
            {
                dst = vsapi->newVideoFrame(dfi, _width, _height, src, core);
            }

            for (int i = 0; i < PlaneCount; ++i)
            {
                dst_height[i] = vsapi->getFrameHeight(dst, i);
                dst_width[i] = vsapi->getFrameWidth(dst, i);
                dst_stride[i] = vsapi->getStride(dst, i) / dfi->bytesPerSample;
                dst_pcount[i] = dst_height[i] * dst_stride[i];
            }
        }
    }

protected:
    // To call these template functions, it is required to include "Conversion.hpp"
    template < typename _Ty >
    void Int2Float(FLType *dst, const _Ty *src,
        PCType height, PCType width, PCType dst_stride, PCType src_stride,
        bool chroma = false, bool full = true, bool clip = false);

    template < typename _Ty >
    void Float2Int(_Ty *dst, const FLType *src,
        PCType height, PCType width, PCType dst_stride, PCType src_stride,
        bool chroma = false, bool full = true, bool clip = true);

    template < typename _Ty >
    void RGB2FloatY(FLType *dst,
        const _Ty *srcR, const _Ty *srcG, const _Ty *srcB,
        PCType height, PCType width, PCType dst_stride, PCType src_stride,
        ColorMatrix matrix, bool full = true, bool clip = false);

    template < typename _Ty >
    void RGB2FloatYUV(FLType *dstY, FLType *dstU, FLType *dstV,
        const _Ty *srcR, const _Ty *srcG, const _Ty *srcB,
        PCType height, PCType width, PCType dst_stride, PCType src_stride,
        ColorMatrix matrix, bool full = true, bool clip = false);

    template < typename _Ty >
    void FloatYUV2RGB(_Ty *dstR, _Ty *dstG, _Ty *dstB,
        const FLType *srcY, const FLType *srcU, const FLType *srcV,
        PCType height, PCType width, PCType dst_stride, PCType src_stride,
        ColorMatrix matrix, bool full = true, bool clip = true);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif