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
