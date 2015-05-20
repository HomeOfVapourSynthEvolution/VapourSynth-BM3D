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


#ifndef BM3D_BASE_HPP_
#define BM3D_BASE_HPP_


#include "BM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class BM3D_Process_Base


template < typename _Ty >
void BM3D_Process_Base::process_core()
{
    if (fi->colorFamily == cmGray || (
        (fi->colorFamily == cmYUV || fi->colorFamily == cmYCoCg)
        && !d.process[1] && !d.process[2]
        ))
    {
        process_core_gray<_Ty>();
    }
    else if (fi->colorFamily == cmYUV || fi->colorFamily == cmYCoCg)
    {
        process_core_yuv<_Ty>();
    }
    else if (fi->colorFamily == cmRGB)
    {
        process_core_rgb<_Ty>();
    }
}


template < typename _Ty >
void BM3D_Process_Base::process_core_gray()
{
    FLType *dstYd = nullptr, *srcYd = nullptr, *refYd = nullptr;

    // Get write/read pointer
    auto dstY = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 0));
    auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 0));
    auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 0));

    // Allocate memory for floating point Y data
    AlignedMalloc(dstYd, dst_pcount[0]);
    AlignedMalloc(srcYd, src_pcount[0]);
    if (d.rdef) AlignedMalloc(refYd, ref_pcount[0]);
    else refYd = srcYd;

    // Convert src and ref from integer Y data to floating point Y data
    Int2Float(srcYd, srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, true, false);
    if (d.rdef) Int2Float(refYd, refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, true, false);

    // Execute kernel
    Kernel(dstYd, srcYd, refYd);

    // Convert dst from floating point Y data to integer Y data
    Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, true, true);

    // Free memory for floating point Y data
    AlignedFree(dstYd);
    AlignedFree(srcYd);
    if (d.rdef) AlignedFree(refYd);
}

template <>
void BM3D_Process_Base::process_core_gray<FLType>()
{
    // Get write/read pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0));
    auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(src, 0));
    auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(ref, 0));

    // Execute kernel
    Kernel(dstY, srcY, refY);
}


template < typename _Ty >
void BM3D_Process_Base::process_core_yuv()
{
    FLType *dstYd = nullptr, *dstUd = nullptr, *dstVd = nullptr;
    FLType *srcYd = nullptr, *srcUd = nullptr, *srcVd = nullptr;
    FLType *refYd = nullptr, *refUd = nullptr, *refVd = nullptr;

    // Get write/read pointer
    auto dstY = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 0));
    auto dstU = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 1));
    auto dstV = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 2));

    auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 0));
    auto srcU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 1));
    auto srcV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 2));

    auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 0));
    auto refU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 1));
    auto refV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 2));

    // Allocate memory for floating point YUV data
    if (d.process[0]) AlignedMalloc(dstYd, dst_pcount[0]);
    if (d.process[1]) AlignedMalloc(dstUd, dst_pcount[1]);
    if (d.process[2]) AlignedMalloc(dstVd, dst_pcount[2]);

    if (d.process[0] || !d.rdef) AlignedMalloc(srcYd, src_pcount[0]);
    if (d.process[1]) AlignedMalloc(srcUd, src_pcount[1]);
    if (d.process[2]) AlignedMalloc(srcVd, src_pcount[2]);

    if (d.rdef)
    {
        AlignedMalloc(refYd, ref_pcount[0]);
        if (d.wiener && d.process[1]) AlignedMalloc(refUd, ref_pcount[1]);
        if (d.wiener && d.process[2]) AlignedMalloc(refVd, ref_pcount[2]);
    }
    else
    {
        refYd = srcYd;
        refUd = srcUd;
        refVd = srcVd;
    }

    // Convert src and ref from integer YUV data to floating point YUV data
    if (d.process[0] || !d.rdef) Int2Float(srcYd, srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, true, false);
    if (d.process[1]) Int2Float(srcUd, srcU, src_height[1], src_width[1], src_stride[1], src_stride[1], true, true, false);
    if (d.process[2]) Int2Float(srcVd, srcV, src_height[2], src_width[2], src_stride[2], src_stride[2], true, true, false);

    if (d.rdef)
    {
        Int2Float(refYd, refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, true, false);
        if (d.wiener && d.process[1]) Int2Float(refUd, refU, ref_height[1], ref_width[1], ref_stride[1], ref_stride[1], true, true, false);
        if (d.wiener && d.process[2]) Int2Float(refVd, refV, ref_height[2], ref_width[2], ref_stride[2], ref_stride[2], true, true, false);
    }

    // Execute kernel
    Kernel(dstYd, dstUd, dstVd, srcYd, srcUd, srcVd, refYd, refUd, refVd);

    // Convert dst from floating point YUV data to integer YUV data
    if (d.process[0]) Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, true, true);
    if (d.process[1]) Float2Int(dstU, dstUd, dst_height[1], dst_width[1], dst_stride[1], dst_stride[1], true, true, true);
    if (d.process[2]) Float2Int(dstV, dstVd, dst_height[2], dst_width[2], dst_stride[2], dst_stride[2], true, true, true);

    // Free memory for floating point YUV data
    if (d.process[0]) AlignedFree(dstYd);
    if (d.process[1]) AlignedFree(dstUd);
    if (d.process[2]) AlignedFree(dstVd);

    if (d.process[0] || !d.rdef) AlignedFree(srcYd);
    if (d.process[1]) AlignedFree(srcUd);
    if (d.process[2]) AlignedFree(srcVd);

    if (d.rdef)
    {
        AlignedFree(refYd);
        if (d.wiener && d.process[1]) AlignedFree(refUd);
        if (d.wiener && d.process[2]) AlignedFree(refVd);
    }
}

template <>
void BM3D_Process_Base::process_core_yuv<FLType>()
{
    // Get write/read pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0));
    auto dstU = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 1));
    auto dstV = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 2));

    auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(src, 0));
    auto srcU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(src, 1));
    auto srcV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(src, 2));

    auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(ref, 0));
    auto refU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(ref, 1));
    auto refV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(ref, 2));

    // Execute kernel
    Kernel(dstY, dstU, dstV, srcY, srcU, srcV, refY, refU, refV);
}


template < typename _Ty >
void BM3D_Process_Base::process_core_rgb()
{
    FLType *dstYd = nullptr, *dstUd = nullptr, *dstVd = nullptr;
    FLType *srcYd = nullptr, *srcUd = nullptr, *srcVd = nullptr;
    FLType *refYd = nullptr, *refUd = nullptr, *refVd = nullptr;

    // Get write/read pointer
    auto dstR = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 0));
    auto dstG = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 1));
    auto dstB = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 2));

    auto srcR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 0));
    auto srcG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 1));
    auto srcB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 2));

    auto refR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 0));
    auto refG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 1));
    auto refB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 2));

    // Allocate memory for floating point YUV data
    AlignedMalloc(dstYd, dst_pcount[0]);
    AlignedMalloc(dstUd, dst_pcount[1]);
    AlignedMalloc(dstVd, dst_pcount[2]);

    AlignedMalloc(srcYd, src_pcount[0]);
    AlignedMalloc(srcUd, src_pcount[1]);
    AlignedMalloc(srcVd, src_pcount[2]);

    if (d.rdef)
    {
        AlignedMalloc(refYd, ref_pcount[0]);
        if (d.wiener) AlignedMalloc(refUd, ref_pcount[1]);
        if (d.wiener) AlignedMalloc(refVd, ref_pcount[2]);
    }
    else
    {
        refYd = srcYd;
        refUd = srcUd;
        refVd = srcVd;
    }

    // Convert src and ref from RGB data to floating point YUV data
    RGB2FloatYUV(srcYd, srcUd, srcVd, srcR, srcG, srcB,
        src_height[0], src_width[0], src_stride[0], src_stride[0],
        ColorMatrix::OPP, true);

    if (d.rdef)
    {
        if (d.wiener)
        {
            RGB2FloatYUV(refYd, refUd, refVd, refR, refG, refB,
                ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                ColorMatrix::OPP, true);
        }
        else
        {
            RGB2FloatY(refYd, refR, refG, refB,
                ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                ColorMatrix::OPP, true);
        }
    }

    // Execute kernel
    Kernel(dstYd, dstUd, dstVd, srcYd, srcUd, srcVd, refYd, refUd, refVd);

    // Convert dst from floating point YUV data to RGB data
    FloatYUV2RGB(dstR, dstG, dstB, dstYd, dstUd, dstVd,
        dst_height[0], dst_width[0], dst_stride[0], dst_stride[0],
        ColorMatrix::OPP, true);

    // Free memory for floating point YUV data
    AlignedFree(dstYd);
    AlignedFree(dstUd);
    AlignedFree(dstVd);

    AlignedFree(srcYd);
    AlignedFree(srcUd);
    AlignedFree(srcVd);

    if (d.rdef)
    {
        AlignedFree(refYd);
        if (d.wiener) AlignedFree(refUd);
        if (d.wiener) AlignedFree(refVd);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
