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


#include "BM3D_Final.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class BM3D_Final_Data


int BM3D_Final_Data::arguments_process(const VSMap *in, VSMap *out)
{
    if (_Mybase::arguments_process(in, out))
    {
        return 1;
    }

    // Initialize filter data for empirical Wiener filtering
    init_filter_data();

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class BM3D_Final_Process


void BM3D_Final_Process::CollaborativeFilter(int plane,
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

    // Construct source group and reference group guided by matched pos code
    block_group srcGroup(src, src_stride[plane], code, GroupSize, d.para.BlockSize, d.para.BlockSize);
    block_group refGroup(ref, ref_stride[plane], code, GroupSize, d.para.BlockSize, d.para.BlockSize);

    // Initialize L2-norm of Wiener coefficients
    FLType L2Wiener = 0;

    // Apply forward 3D transform to the source group and the reference group
    d.f[plane].fp[GroupSize - 1].execute_r2r(srcGroup.data(), srcGroup.data());
    d.f[plane].fp[GroupSize - 1].execute_r2r(refGroup.data(), refGroup.data());

    // Apply empirical Wiener filtering to the source group guided by the reference group
    const FLType sigmaSquare = d.f[plane].wienerSigmaSqr[GroupSize - 1];

    auto srcp = srcGroup.data();
    auto refp = refGroup.data();
    const auto upper = srcp + srcGroup.size();

#if defined(__SSE2__)
    static const ptrdiff_t simd_step = 4;
    const ptrdiff_t simd_residue = srcGroup.size() % simd_step;
    const ptrdiff_t simd_width = srcGroup.size() - simd_residue;

    const __m128 sgm_sqr = _mm_set_ps1(sigmaSquare);
    __m128 l2wiener_sum = _mm_setzero_ps();

    for (const auto upper1 = srcp + simd_width; srcp < upper1; srcp += simd_step, refp += simd_step)
    {
        const __m128 s1 = _mm_load_ps(srcp);
        const __m128 r1 = _mm_load_ps(refp);
        const __m128 r1sqr = _mm_mul_ps(r1, r1);

        const __m128 wiener = _mm_mul_ps(r1sqr, _mm_rcp_ps(_mm_add_ps(r1sqr, sgm_sqr)));

        const __m128 d1 = _mm_mul_ps(s1, wiener);
        _mm_store_ps(srcp, d1);
        l2wiener_sum = _mm_add_ps(l2wiener_sum, _mm_mul_ps(wiener, wiener));
    }

    alignas(16) FLType l2wiener_sum_f32[4];
    _mm_store_ps(l2wiener_sum_f32, l2wiener_sum);
    L2Wiener += l2wiener_sum_f32[0] + l2wiener_sum_f32[1] + l2wiener_sum_f32[2] + l2wiener_sum_f32[3];
#endif

    for (; srcp < upper; ++srcp, ++refp)
    {
        const FLType refSquare = *refp * *refp;
        const FLType wienerCoef = refSquare / (refSquare + sigmaSquare);
        *srcp *= wienerCoef;
        L2Wiener += wienerCoef * wienerCoef;
    }

    // Apply backward 3D transform to the filtered group
    d.f[plane].bp[GroupSize - 1].execute_r2r(srcGroup.data(), srcGroup.data());

    // Calculate weight for the filtered group
    // Also include the normalization factor to compensate for the amplification introduced in 3D transform
    FLType denWeight = L2Wiener <= 0 ? 1 : FLType(1) / L2Wiener;
    FLType numWeight = static_cast<FLType>(denWeight / d.f[plane].finalAMP[GroupSize - 1]);

    // Store the weighted filtered group to the numerator part of the final estimation
    // Store the weight to the denominator part of the final estimation
    srcGroup.AddTo(ResNum, dst_stride[plane], numWeight);
    srcGroup.CountTo(ResDen, dst_stride[plane], denWeight);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
