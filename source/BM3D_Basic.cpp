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


#include "BM3D_Basic.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class BM3D_Basic_Data


int BM3D_Basic_Data::arguments_process(const VSMap *in, VSMap *out)
{
    if (_Mybase::arguments_process(in, out))
    {
        return 1;
    }

    int error;

    // hard_thr - float
    para.lambda = vsapi->propGetFloat(in, "hard_thr", 0, &error);

    if (error)
    {
        para.lambda = para_default.lambda;
    }
    else if (para.lambda <= 0)
    {
        setError(out, "Invalid \"hard_thr\" assigned, must be a positive floating point number");
        return 1;
    }

    // Initialize filter data for hard-threshold filtering
    init_filter_data();

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class BM3D_Basic_Process


void BM3D_Basic_Process::CollaborativeFilter(int plane,
    FLType *ResNum, FLType *ResDen,
    const FLType *src, const FLType *ref,
    const PosPairCode &code) const
{
    PCType GroupSize = static_cast<PCType>(code.size());
    // When para.GroupSize > 0, limit GroupSize up to para.GroupSize
    if (d.para.GroupSize > 0 && GroupSize > d.para.GroupSize)
    {
        GroupSize = d.para.GroupSize;
    }

    // Construct source group guided by matched pos code
    block_group srcGroup(src, src_stride[plane], code, GroupSize, d.para.BlockSize, d.para.BlockSize);

    // Initialize retianed coefficients of hard threshold filtering
    int retainedCoefs = 0;

    // Apply forward 3D transform to the source group
    d.f[plane].fp[GroupSize - 1].execute_r2r(srcGroup.data(), srcGroup.data());

    // Apply hard-thresholding to the source group
    auto srcp = srcGroup.data();
    auto thrp = d.f[plane].thrTable[GroupSize - 1].get();
    const auto upper = srcp + srcGroup.size();

#if defined(__SSE2__)
    static const ptrdiff_t simd_step = 4;
    const ptrdiff_t simd_residue = srcGroup.size() % simd_step;
    const ptrdiff_t simd_width = srcGroup.size() - simd_residue;

    __m128i cmp_sum = _mm_setzero_si128();

    for (const auto upper1 = srcp + simd_width; srcp < upper1; srcp += simd_step, thrp += simd_step)
    {
        const __m128 s1 = _mm_load_ps(srcp);
        const __m128 t1 = _mm_load_ps(thrp);

        const __m128 s1abs = _mm_abs_ps(s1);
        const __m128 cmp = _mm_cmpgt_ps(s1abs, t1);

        const __m128 d1 = _mm_and_ps(cmp, s1);
        _mm_store_ps(srcp, d1);
        cmp_sum = _mm_sub_epi32(cmp_sum, _mm_castps_si128(cmp));
    }

    alignas(16) int32_t cmp_sum_i32[4];
    _mm_store_si128(reinterpret_cast<__m128i *>(cmp_sum_i32), cmp_sum);
    retainedCoefs += cmp_sum_i32[0] + cmp_sum_i32[1] + cmp_sum_i32[2] + cmp_sum_i32[3];
#endif

    for (; srcp < upper; ++srcp, ++thrp)
    {
        if (*srcp > *thrp || *srcp < -*thrp)
        {
            ++retainedCoefs;
        }
        else
        {
            *srcp = 0;
        }
    }

    // Apply backward 3D transform to the filtered group
    d.f[plane].bp[GroupSize - 1].execute_r2r(srcGroup.data(), srcGroup.data());

    // Calculate weight for the filtered group
    // Also include the normalization factor to compensate for the amplification introduced in 3D transform
    FLType denWeight = retainedCoefs < 1 ? 1 : FLType(1) / static_cast<FLType>(retainedCoefs);
    FLType numWeight = static_cast<FLType>(denWeight / d.f[plane].finalAMP[GroupSize - 1]);

    // Store the weighted filtered group to the numerator part of the basic estimation
    // Store the weight to the denominator part of the basic estimation
    srcGroup.AddTo(ResNum, dst_stride[plane], numWeight);
    srcGroup.CountTo(ResDen, dst_stride[plane], denWeight);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
