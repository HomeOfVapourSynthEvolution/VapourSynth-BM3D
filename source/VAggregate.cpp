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


#include "VAggregate.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VAggregate_Data


int VAggregate_Data::arguments_process(const VSMap *in, VSMap *out)
{
    try
    {
        int error;

        // input - clip
        node = vsapi->propGetNode(in, "input", 0, nullptr);
        vi = vsapi->getVideoInfo(node);

        if (!isConstantFormat(vi))
        {
            throw std::string("Invalid input clip, only constant format input supported");
        }
        if (vi->format->sampleType != stFloat || vi->format->bitsPerSample != 32)
        {
            throw std::string("Invalid input clip, only accept 32 bit float format clip from bm3d.VBasic or bm3d.VFinal");
        }
        if (vi->format->colorFamily == cmRGB)
        {
            throw std::string("Invalid input clip, must be of Gray, YUV or YCoCg color family");
        }

        // radius - int
        radius = int64ToIntS(vsapi->propGetInt(in, "radius", 0, &error));

        if (error)
        {
            radius = 1;
        }
        else if (radius < 1 || radius > 16)
        {
            throw std::string("Invalid \"radius\" assigned, must be an integer in [1, 16]");
        }

        // sample - int
        sample = static_cast<VSSampleType>(vsapi->propGetInt(in, "sample", 0, &error));

        if (error)
        {
            sample = stInteger;
        }
        else if (sample != stInteger && sample != stFloat)
        {
            throw std::string("Invalid \'sample\' assigned, must be 0 (integer sample type) or 1 (float sample type)");
        }
    }
    catch (const std::string &error_msg)
    {
        setError(out, error_msg.c_str());
        return 1;
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VAggregate_Process


void VAggregate_Process::Kernel(FLType *dst,
    std::vector<const FLType *> ResNum, std::vector<const FLType *> ResDen)
{
    // The filtered blocks are sumed and averaged to form the final filtered image
    LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], src_stride[0], [&](PCType i0, PCType i1)
    {
        FLType num = 0;
        FLType den = 0;

        for (int f = 0; f < frames; ++f)
        {
            num += ResNum[f][i1];
            den += ResDen[f][i1];
        }

        dst[i0] = num / den;
    });
}


void VAggregate_Process::Kernel(FLType *dstY, FLType *dstU, FLType *dstV,
    std::vector<const FLType *> ResNumY, std::vector<const FLType *> ResDenY,
    std::vector<const FLType *> ResNumU, std::vector<const FLType *> ResDenU,
    std::vector<const FLType *> ResNumV, std::vector<const FLType *> ResDenV)
{
    // The filtered blocks are sumed and averaged to form the final filtered image
    if (process_plane[0]) LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], src_stride[0], [&](PCType i0, PCType i1)
    {
        FLType num = 0;
        FLType den = 0;

        for (int f = 0; f < frames; ++f)
        {
            num += ResNumY[f][i1];
            den += ResDenY[f][i1];
        }

        dstY[i0] = num / den;
    });

    if (process_plane[1]) LOOP_VH(dst_height[1], dst_width[1], dst_stride[1], src_stride[1], [&](PCType i0, PCType i1)
    {
        FLType num = 0;
        FLType den = 0;

        for (int f = 0; f < frames; ++f)
        {
            num += ResNumU[f][i1];
            den += ResDenU[f][i1];
        }

        dstU[i0] = num / den;
    });

    if (process_plane[2]) LOOP_VH(dst_height[2], dst_width[2], dst_stride[2], src_stride[2], [&](PCType i0, PCType i1)
    {
        FLType num = 0;
        FLType den = 0;

        for (int f = 0; f < frames; ++f)
        {
            num += ResNumV[f][i1];
            den += ResDenV[f][i1];
        }

        dstV[i0] = num / den;
    });
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class VAggregate_Process


template < typename _Dt1 >
void VAggregate_Process::process_core()
{
    if (fi->colorFamily == cmGray || (
        (fi->colorFamily == cmYUV || fi->colorFamily == cmYCoCg)
        && !process_plane[1] && !process_plane[2]
        ))
    {
        process_core_gray<_Dt1>();
    }
    else if (fi->colorFamily == cmYUV || fi->colorFamily == cmYCoCg)
    {
        process_core_yuv<_Dt1>();
    }
}


template < typename _Dt1 >
void VAggregate_Process::process_core_gray()
{
    FLType *dstYd;

    std::vector<const FLType *> ResNumY, ResDenY;

    // Get write pointer
    auto dstY = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 0));

    for (int i = 0, o = d.radius - b_offset; i < frames; ++i, --o)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));

        // Store pointer to floating point intermediate Y data into corresponding result vector
        ResNumY.push_back(srcY + src_pcount[0] * (o * 2));
        ResDenY.push_back(srcY + src_pcount[0] * (o * 2 + 1));
    }

    // Allocate memory for floating point Y data
    AlignedMalloc(dstYd, dst_pcount[0]);

    // Execute kernel
    Kernel(dstYd, ResNumY, ResDenY);

    // Convert dst from floating point Y data to integer Y data
    Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, full, !isFloat(_Dt1));

    // Free memory for floating point YUV data
    AlignedFree(dstYd);
}

template <>
void VAggregate_Process::process_core_gray<FLType>()
{
    std::vector<const FLType *> ResNumY, ResDenY;

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0));

    for (int i = 0, o = d.radius - b_offset; i < frames; ++i, --o)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));

        // Store pointer to floating point intermediate Y data into corresponding result vector
        ResNumY.push_back(srcY + src_pcount[0] * (o * 2));
        ResDenY.push_back(srcY + src_pcount[0] * (o * 2 + 1));
    }

    // Execute kernel
    Kernel(dstY, ResNumY, ResDenY);
}


template < typename _Dt1 >
void VAggregate_Process::process_core_yuv()
{
    FLType *dstYd = nullptr, *dstUd = nullptr, *dstVd = nullptr;

    std::vector<const FLType *> ResNumY, ResDenY;
    std::vector<const FLType *> ResNumU, ResDenU;
    std::vector<const FLType *> ResNumV, ResDenV;

    // Get write pointer
    auto dstY = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 0));
    auto dstU = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 1));
    auto dstV = reinterpret_cast<_Dt1 *>(vsapi->getWritePtr(dst, 2));

    for (int i = 0, o = d.radius - b_offset; i < frames; ++i, --o)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));
        auto srcU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 1));
        auto srcV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 2));

        // Store pointer to floating point intermediate YUV data into corresponding result vector
        ResNumY.push_back(srcY + src_pcount[0] * (o * 2));
        ResNumU.push_back(srcU + src_pcount[1] * (o * 2));
        ResNumV.push_back(srcV + src_pcount[2] * (o * 2));

        ResDenY.push_back(srcY + src_pcount[0] * (o * 2 + 1));
        ResDenU.push_back(srcU + src_pcount[1] * (o * 2 + 1));
        ResDenV.push_back(srcV + src_pcount[2] * (o * 2 + 1));
    }

    // Allocate memory for floating point YUV data
    if (process_plane[0]) AlignedMalloc(dstYd, dst_pcount[0]);
    if (process_plane[1]) AlignedMalloc(dstUd, dst_pcount[1]);
    if (process_plane[2]) AlignedMalloc(dstVd, dst_pcount[2]);

    // Execute kernel
    Kernel(dstYd, dstUd, dstVd, ResNumY, ResDenY, ResNumU, ResDenU, ResNumV, ResDenV);

    // Convert dst from floating point YUV data to integer YUV data
    if (process_plane[0]) Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, full, !isFloat(_Dt1));
    if (process_plane[1]) Float2Int(dstU, dstUd, dst_height[1], dst_width[1], dst_stride[1], dst_stride[1], true, full, !isFloat(_Dt1));
    if (process_plane[2]) Float2Int(dstV, dstVd, dst_height[2], dst_width[2], dst_stride[2], dst_stride[2], true, full, !isFloat(_Dt1));

    // Free memory for floating point YUV data
    if (process_plane[0]) AlignedFree(dstYd);
    if (process_plane[1]) AlignedFree(dstUd);
    if (process_plane[2]) AlignedFree(dstVd);
}

template <>
void VAggregate_Process::process_core_yuv<FLType>()
{
    std::vector<const FLType *> ResNumY, ResDenY;
    std::vector<const FLType *> ResNumU, ResDenU;
    std::vector<const FLType *> ResNumV, ResDenV;

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0));
    auto dstU = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 1));
    auto dstV = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 2));

    for (int i = 0, o = d.radius - b_offset; i < frames; ++i, --o)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));
        auto srcU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 1));
        auto srcV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 2));

        // Store pointer to floating point intermediate YUV data into corresponding result vector
        ResNumY.push_back(srcY + src_pcount[0] * (o * 2));
        ResNumU.push_back(srcU + src_pcount[1] * (o * 2));
        ResNumV.push_back(srcV + src_pcount[2] * (o * 2));

        ResDenY.push_back(srcY + src_pcount[0] * (o * 2 + 1));
        ResDenU.push_back(srcU + src_pcount[1] * (o * 2 + 1));
        ResDenV.push_back(srcV + src_pcount[2] * (o * 2 + 1));
    }

    // Execute kernel
    Kernel(dstY, dstU, dstV, ResNumY, ResDenY, ResNumU, ResDenU, ResNumV, ResDenV);
}


void VAggregate_Process::process_coreS()
{
    if (d.sample == stInteger)
    {
        process_core<uint16_t>();
    }
    else
    {
        process_core<FLType>();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
