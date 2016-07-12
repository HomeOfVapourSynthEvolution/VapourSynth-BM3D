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


#ifndef VAGGREGATE_HPP_
#define VAGGREGATE_HPP_


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
inline void VAggregate_Process::process_core_gray<FLType>()
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
inline void VAggregate_Process::process_core_yuv<FLType>()
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
