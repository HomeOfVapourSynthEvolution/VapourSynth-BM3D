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


#include "VAggregate.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VAggregate_Data


int VAggregate_Data::arguments_process(const VSMap *in, VSMap *out)
{
    int error;

    // input - clip
    node = vsapi->propGetNode(in, "input", 0, nullptr);
    vi = vsapi->getVideoInfo(node);

    if (!isConstantFormat(vi))
    {
        setError(out, "Invalid input clip, only constant format input supported");
        return 1;
    }
    if (vi->format->sampleType != stFloat || vi->format->bitsPerSample != 32)
    {
        setError(out, "Invalid input clip, only accept 32 bit float format clip from bm3d.VBasic or bm3d.VFinal");
        return 1;
    }
    if (vi->format->colorFamily == cmRGB)
    {
        setError(out, "Invalid input clip, must be of Gray, YUV or YCoCg color family");
        return 1;
    }

    // radius - int
    radius = int64ToIntS(vsapi->propGetInt(in, "radius", 0, &error));

    if (error)
    {
        radius = 1;
    }
    else if (radius < 1 || radius > 16)
    {
        setError(out, "Invalid \"radius\" assigned, must be an integer in [1, 16]");
        return 1;
    }

    // sample - int
    sample = static_cast<VSSampleType>(vsapi->propGetInt(in, "sample", 0, &error));

    if (error)
    {
        sample = stInteger;
    }
    else if (sample != stInteger && sample != stFloat)
    {
        setError(out, "Invalid \'sample\' assigned, must be 0 (integer sample type) or 1 (float sample type)");
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
