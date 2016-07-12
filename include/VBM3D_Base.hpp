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


#ifndef VBM3D_BASE_HPP_
#define VBM3D_BASE_HPP_


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class VBM3D_Process_Base


template < typename _Ty >
void VBM3D_Process_Base::process_core()
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
void VBM3D_Process_Base::process_core_gray()
{
    std::vector<FLType *> dstYv;
    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> refYv;

    std::vector<FLType *> srcYd(frames, nullptr), refYd(frames, nullptr);

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 0));
        auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 0));

        // Allocate memory for floating point Y data
        AlignedMalloc(srcYd[i], src_pcount[0]);
        if (d.rdef) AlignedMalloc(refYd[i], ref_pcount[0]);
        else refYd[i] = srcYd[i];

        // Convert src and ref from integer Y data to floating point Y data
        Int2Float(srcYd[i], srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
        if (d.rdef) Int2Float(refYd[i], refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);

        // Store pointer to floating point Y data into corresponding frame of the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        srcYv.push_back(srcYd[i]);
        refYv.push_back(refYd[i]);
    }

    // Execute kernel
    Kernel(dstYv, srcYv, refYv);

    // Free memory for floating point Y data
    for (int i = 0; i < frames; ++i)
    {
        AlignedFree(srcYd[i]);
        if (d.rdef) AlignedFree(refYd[i]);
    }
}

template <>
inline void VBM3D_Process_Base::process_core_gray<FLType>()
{
    std::vector<FLType *> dstYv;
    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> refYv;

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));
        auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_ref[i], 0));

        // Store pointer to floating point Y data into corresponding frame of the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        srcYv.push_back(srcY);
        refYv.push_back(refY);
    }

    // Execute kernel
    Kernel(dstYv, srcYv, refYv);
}


template < typename _Ty >
void VBM3D_Process_Base::process_core_yuv()
{
    std::vector<FLType *> dstYv;
    std::vector<FLType *> dstUv;
    std::vector<FLType *> dstVv;

    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> srcUv;
    std::vector<const FLType *> srcVv;

    std::vector<const FLType *> refYv;
    std::vector<const FLType *> refUv;
    std::vector<const FLType *> refVv;

    std::vector<FLType *> srcYd(frames, nullptr), srcUd(frames, nullptr), srcVd(frames, nullptr);
    std::vector<FLType *> refYd(frames, nullptr), refUd(frames, nullptr), refVd(frames, nullptr);

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);
    auto dstU = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 1))
        + dst_pcount[1] * 2 * (d.para.radius + b_offset);
    auto dstV = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 2))
        + dst_pcount[2] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 0));
        auto srcU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 1));
        auto srcV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 2));

        auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 0));
        auto refU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 1));
        auto refV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 2));

        // Allocate memory for floating point YUV data
        if (d.process[0] || !d.rdef) AlignedMalloc(srcYd[i], src_pcount[0]);
        if (d.process[1]) AlignedMalloc(srcUd[i], src_pcount[1]);
        if (d.process[2]) AlignedMalloc(srcVd[i], src_pcount[2]);

        if (d.rdef)
        {
            AlignedMalloc(refYd[i], ref_pcount[0]);
            if (d.wiener && d.process[1]) AlignedMalloc(refUd[i], ref_pcount[1]);
            if (d.wiener && d.process[2]) AlignedMalloc(refVd[i], ref_pcount[2]);
        }
        else
        {
            refYd[i] = srcYd[i];
            refUd[i] = srcUd[i];
            refVd[i] = srcVd[i];
        }

        // Convert src and ref from integer YUV data to floating point YUV data
        if (d.process[0] || !d.rdef) Int2Float(srcYd[i], srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
        if (d.process[1]) Int2Float(srcUd[i], srcU, src_height[1], src_width[1], src_stride[1], src_stride[1], true, full, false);
        if (d.process[2]) Int2Float(srcVd[i], srcV, src_height[2], src_width[2], src_stride[2], src_stride[2], true, full, false);

        if (d.rdef)
        {
            Int2Float(refYd[i], refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);
            if (d.wiener && d.process[1]) Int2Float(refUd[i], refU, ref_height[1], ref_width[1], ref_stride[1], ref_stride[1], true, full, false);
            if (d.wiener && d.process[2]) Int2Float(refVd[i], refV, ref_height[2], ref_width[2], ref_stride[2], ref_stride[2], true, full, false);
        }

        // Store pointer to floating point YUV data into corresponding frame in the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2));

        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2 + 1));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2 + 1));

        srcYv.push_back(srcYd[i]);
        srcUv.push_back(srcUd[i]);
        srcVv.push_back(srcVd[i]);

        refYv.push_back(refYd[i]);
        refUv.push_back(refUd[i]);
        refVv.push_back(refVd[i]);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, refUv, refVv);

    // Free memory for floating point YUV data
    for (int i = 0; i < frames; ++i)
    {
        if (d.process[0] || !d.rdef) AlignedFree(srcYd[i]);
        if (d.process[1]) AlignedFree(srcUd[i]);
        if (d.process[2]) AlignedFree(srcVd[i]);

        if (d.rdef)
        {
            AlignedFree(refYd[i]);
            if (d.wiener && d.process[1]) AlignedFree(refUd[i]);
            if (d.wiener && d.process[2]) AlignedFree(refVd[i]);
        }
    }
}

template <>
inline void VBM3D_Process_Base::process_core_yuv<FLType>()
{
    std::vector<FLType *> dstYv;
    std::vector<FLType *> dstUv;
    std::vector<FLType *> dstVv;

    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> srcUv;
    std::vector<const FLType *> srcVv;

    std::vector<const FLType *> refYv;
    std::vector<const FLType *> refUv;
    std::vector<const FLType *> refVv;

    // Get write/read pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);
    auto dstU = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 1))
        + dst_pcount[1] * 2 * (d.para.radius + b_offset);
    auto dstV = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 2))
        + dst_pcount[2] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));
        auto srcU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 1));
        auto srcV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 2));

        auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_ref[i], 0));
        auto refU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_ref[i], 1));
        auto refV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_ref[i], 2));

        // Store pointer to floating point YUV data into corresponding frame in the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2));

        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2 + 1));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2 + 1));

        srcYv.push_back(srcY);
        srcUv.push_back(srcU);
        srcVv.push_back(srcV);

        refYv.push_back(refY);
        refUv.push_back(refU);
        refVv.push_back(refV);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, refUv, refVv);
}


template < typename _Ty >
void VBM3D_Process_Base::process_core_rgb()
{
    std::vector<FLType *> dstYv;
    std::vector<FLType *> dstUv;
    std::vector<FLType *> dstVv;

    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> srcUv;
    std::vector<const FLType *> srcVv;

    std::vector<const FLType *> refYv;
    std::vector<const FLType *> refUv;
    std::vector<const FLType *> refVv;

    std::vector<FLType *> srcYd(frames, nullptr), srcUd(frames, nullptr), srcVd(frames, nullptr);
    std::vector<FLType *> refYd(frames, nullptr), refUd(frames, nullptr), refVd(frames, nullptr);

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);
    auto dstU = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 1))
        + dst_pcount[1] * 2 * (d.para.radius + b_offset);
    auto dstV = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 2))
        + dst_pcount[2] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 0));
        auto srcG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 1));
        auto srcB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 2));

        auto refR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 0));
        auto refG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 1));
        auto refB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 2));

        // Allocate memory for floating point YUV data
        AlignedMalloc(srcYd[i], src_pcount[0]);
        AlignedMalloc(srcUd[i], src_pcount[1]);
        AlignedMalloc(srcVd[i], src_pcount[2]);

        if (d.rdef)
        {
            AlignedMalloc(refYd[i], ref_pcount[0]);
            if (d.wiener) AlignedMalloc(refUd[i], ref_pcount[1]);
            if (d.wiener) AlignedMalloc(refVd[i], ref_pcount[2]);
        }
        else
        {
            refYd[i] = srcYd[i];
            refUd[i] = srcUd[i];
            refVd[i] = srcVd[i];
        }

        // Convert src and ref from RGB data to floating point YUV data
        RGB2FloatYUV(srcYd[i], srcUd[i], srcVd[i], srcR, srcG, srcB,
            src_height[0], src_width[0], src_stride[0], src_stride[0],
            ColorMatrix::OPP, true, false);

        if (d.rdef)
        {
            if (d.wiener)
            {
                RGB2FloatYUV(refYd[i], refUd[i], refVd[i], refR, refG, refB,
                    ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                    ColorMatrix::OPP, true, false);
            }
            else
            {
                RGB2FloatY(refYd[i], refR, refG, refB,
                    ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                    ColorMatrix::OPP, true, false);
            }
        }

        // Store pointer to floating point YUV data into corresponding frame in the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2));

        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        dstUv.push_back(dstU + dst_pcount[1] * (i * 2 + 1));
        dstVv.push_back(dstV + dst_pcount[2] * (i * 2 + 1));

        srcYv.push_back(srcYd[i]);
        srcUv.push_back(srcUd[i]);
        srcVv.push_back(srcVd[i]);

        refYv.push_back(refYd[i]);
        refUv.push_back(refUd[i]);
        refVv.push_back(refVd[i]);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, refUv, refVv);

    // Free memory for floating point YUV data
    for (int i = 0; i < frames; ++i)
    {
        AlignedFree(srcYd[i]);
        AlignedFree(srcUd[i]);
        AlignedFree(srcVd[i]);

        if (d.rdef)
        {
            AlignedFree(refYd[i]);
            if (d.wiener) AlignedFree(refUd[i]);
            if (d.wiener) AlignedFree(refVd[i]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
