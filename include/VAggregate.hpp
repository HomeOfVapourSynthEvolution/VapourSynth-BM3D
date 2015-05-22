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


#ifndef VAGGREGATE_HPP_
#define VAGGREGATE_HPP_


#include "VAggregate.h"


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

    for (int i = 0, o = d.radius + f_offset; i < frames; ++i, --o)
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

    for (int i = 0, o = d.radius + f_offset; i < frames; ++i, --o)
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

    for (int i = 0, o = d.radius + f_offset; i < frames; ++i, --o)
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
