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


#include "VBM3D_Final.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VBM3D_Final_Data


int VBM3D_Final_Data::arguments_process(const VSMap *in, VSMap *out)
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
// Functions of class VBM3D_Final_Process


void VBM3D_Final_Process::CollaborativeFilter(int plane,
    const std::vector<FLType *> &ResNum, const std::vector<FLType *> &ResDen,
    const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref,
    const Pos3PairCode &code) const
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

    Block_For_each(srcGroup, refGroup, [&](FLType &x, FLType y)
    {
        FLType ySquare = y * y;
        FLType wienerCoef = ySquare / (ySquare + sigmaSquare);
        x *= wienerCoef;
        L2Wiener += wienerCoef * wienerCoef;
    });

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
