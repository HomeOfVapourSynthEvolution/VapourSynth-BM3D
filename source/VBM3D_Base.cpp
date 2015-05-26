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


#include "VBM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of struct VBM3D_Para


VBM3D_Para::VBM3D_Para(bool _wiener, std::string _profile)
    : _Mybase(_wiener, _profile)
{
    radius = 3;
    GroupSize = 8;
    BMrange = 12;
    PSnum = 2;
    PSstep = 1;

    if (!wiener)
    {
        PSrange = 5;
    }
    else
    {
        PSrange = 6;
    }

    if (profile == "fast")
    {
        radius = 1;
        BMrange = 7;

        if (!wiener)
        {
            PSrange = 4;
        }
        else
        {
            PSrange = 5;
        }
    }
    else if (profile == "lc")
    {
        radius = 2;
        BMrange = 9;

        if (!wiener)
        {
            PSrange = 4;
        }
        else
        {
            PSrange = 5;
        }
    }
    else if (profile == "high")
    {
        radius = 4;
        BMrange = 16;

        if (!wiener)
        {
            PSrange = 7;
        }
        else
        {
            PSrange = 8;
        }
    }
    else if (profile == "vn")
    {
        radius = 4;
        GroupSize = 16;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VBM3D_Data_Base


int VBM3D_Data_Base::arguments_process(const VSMap *in, VSMap *out)
{
    int error;
    int m;

    // input - clip
    node = vsapi->propGetNode(in, "input", 0, nullptr);
    vi = vsapi->getVideoInfo(node);

    if (!isConstantFormat(vi))
    {
        setError(out, "Invalid input clip, only constant format input supported");
        return 1;
    }
    if (vi->format->sampleType == stFloat && vi->format->bitsPerSample != 32)
    {
        setError(out, "Invalid input clip, only 8-16 bit int or 32 bit float formats supported");
        return 1;
    }

    // ref - clip
    rnode = vsapi->propGetNode(in, "ref", 0, &error);

    if (error)
    {
        rdef = false;
        rnode = node;
        rvi = vi;
    }
    else
    {
        rdef = true;
        rvi = vsapi->getVideoInfo(rnode);

        if (!isConstantFormat(rvi))
        {
            setError(out, "Invalid clip \"ref\", only constant format input supported");
            return 1;
        }
        if (rvi->format != vi->format)
        {
            setError(out, "input clip and clip \"ref\" must be of the same format");
            return 1;
        }
        if (vi->width != rvi->width || vi->height != rvi->height)
        {
            setError(out, "input clip and clip \"ref\" must be of the same width and height");
            return 1;
        }
        if (vi->numFrames != rvi->numFrames)
        {
            setError(out, "input clip and clip \"ref\" must have the same number of frames");
            return 1;
        }
    }

    // profile - data
    auto profile = vsapi->propGetData(in, "profile", 0, &error);

    if (error)
    {
        para.profile = para_default.profile;
    }
    else
    {
        para.profile = profile;
    }

    if (para.profile != "fast" && para.profile != "lc" && para.profile != "np"
        && para.profile != "high" && para.profile != "vn")
    {
        setError(out, "Unrecognized \"profile\" specified, should be \"fast\", \"lc\", \"np\", \"high\" or \"vn\"\n");
        return 1;
    }

    get_default_para(para.profile);

    // sigma - float[]
    m = vsapi->propNumElements(in, "sigma");

    if (m > 0)
    {
        int i;

        if (m > 3) m = 3;

        for (i = 0; i < m; ++i)
        {
            para.sigma[i] = vsapi->propGetFloat(in, "sigma", i, nullptr);

            if (para.sigma[i] < 0)
            {
                setError(out, "Invalid \"sigma\" assigned, must be a non-negative floating point number");
                return 1;
            }
        }

        for (; i < 3; ++i)
        {
            para.sigma[i] = para.sigma[i - 1];
        }
    }
    else
    {
        para.sigma = para_default.sigma;
    }

    // radius - int
    para.radius = int64ToIntS(vsapi->propGetInt(in, "radius", 0, &error));

    if (error)
    {
        para.radius = para_default.radius;
    }
    else if (para.radius < 1 || para.radius > 16)
    {
        setError(out, "Invalid \"radius\" assigned, must be an integer in [1, 16]");
        return 1;
    }

    // block_size - int
    para.BlockSize = int64ToIntS(vsapi->propGetInt(in, "block_size", 0, &error));

    if (error)
    {
        para.BlockSize = para_default.BlockSize;
    }
    else if (para.BlockSize < 1 || para.BlockSize > 64)
    {
        setError(out, "Invalid \"block_size\" assigned, must be an integer in [1, 64]");
        return 1;
    }
    else if (para.BlockSize > vi->width || para.BlockSize > vi->height)
    {
        setError(out, "Invalid \"block_size\" assigned, must not exceed width or height of the frame");
        return 1;
    }

    // block_step - int
    para.BlockStep = int64ToIntS(vsapi->propGetInt(in, "block_step", 0, &error));

    if (error)
    {
        para.BlockStep = para_default.BlockStep;
    }
    else if (para.BlockStep < 1 || para.BlockStep > para.BlockSize)
    {
        setError(out, "Invalid \"block_step\" assigned, must be an integer in [1, block_size]");
        return 1;
    }

    // group_size - int
    para.GroupSize = int64ToIntS(vsapi->propGetInt(in, "group_size", 0, &error));

    if (error)
    {
        para.GroupSize = para_default.GroupSize;
    }
    else if (para.GroupSize < 1 || para.GroupSize > 256)
    {
        setError(out, "Invalid \"group_size\" assigned, must be an integer in [1, 256]");
        return 1;
    }

    // bm_range - int
    para.BMrange = int64ToIntS(vsapi->propGetInt(in, "bm_range", 0, &error));

    if (error)
    {
        para.BMrange = para_default.BMrange;
    }
    else if (para.BMrange < 1)
    {
        setError(out, "Invalid \"bm_range\" assigned, must be a positive integer");
        return 1;
    }

    // bm_step - int
    para.BMstep = int64ToIntS(vsapi->propGetInt(in, "bm_step", 0, &error));

    if (error)
    {
        para.BMstep = para_default.BMstep;
    }
    else if (para.BMstep < 1 || para.BMstep > para.BMrange)
    {
        setError(out, "Invalid \"bm_step\" assigned, must be an integer in [1, bm_range]");
        return 1;
    }

    // ps_num - int
    para.PSnum = int64ToIntS(vsapi->propGetInt(in, "ps_num", 0, &error));

    if (error)
    {
        para.PSnum = para_default.PSnum;
    }
    else if (para.PSnum < 1 || para.PSnum > para.GroupSize)
    {
        setError(out, "Invalid \"ps_num\" assigned, must be an integer in [1, group_size]");
        return 1;
    }

    // ps_range - int
    para.PSrange = int64ToIntS(vsapi->propGetInt(in, "ps_range", 0, &error));

    if (error)
    {
        para.PSrange = para_default.PSrange;
    }
    else if (para.PSrange < 1)
    {
        setError(out, "Invalid \"ps_range\" assigned, must be a positive integer");
        return 1;
    }

    // ps_step - int
    para.PSstep = int64ToIntS(vsapi->propGetInt(in, "ps_step", 0, &error));

    if (error)
    {
        para.PSstep = para_default.PSstep;
    }
    else if (para.PSstep < 1 || para.PSstep > para.PSrange)
    {
        setError(out, "Invalid \"ps_step\" assigned, must be an integer in [1, ps_range]");
        return 1;
    }

    // th_mse - float
    para.thMSE = vsapi->propGetFloat(in, "th_mse", 0, &error);

    if (error)
    {
        para.thMSE_Default();
    }
    else if (para.thMSE <= 0)
    {
        setError(out, "Invalid \"th_mse\" assigned, must be a positive floating point number");
        return 1;
    }

    // matrix - int
    matrix = static_cast<ColorMatrix>(vsapi->propGetInt(in, "matrix", 0, &error));

    if (vi->format->colorFamily == cmRGB)
    {
        matrix = ColorMatrix::OPP;
    }
    else if (vi->format->colorFamily == cmYCoCg)
    {
        matrix = ColorMatrix::YCgCo;
    }
    else if (error || matrix == ColorMatrix::Unspecified)
    {
        matrix = ColorMatrix_Default(vi->width, vi->height);
    }
    else if (matrix != ColorMatrix::GBR && matrix != ColorMatrix::bt709
        && matrix != ColorMatrix::fcc && matrix != ColorMatrix::bt470bg && matrix != ColorMatrix::smpte170m
        && matrix != ColorMatrix::smpte240m && matrix != ColorMatrix::YCgCo && matrix != ColorMatrix::bt2020nc
        && matrix != ColorMatrix::bt2020c && matrix != ColorMatrix::OPP)
    {
        setError(out, "Unsupported \"matrix\" specified");
        return 1;
    }

    // process
    for (int i = 0; i < VSMaxPlaneCount; i++)
    {
        if (para.sigma[i] == 0)
        {
            process[i] = 0;
        }
    }

    if (process[1] || process[2])
    {
        if (vi->format->subSamplingH || vi->format->subSamplingW)
        {
            setError(out, "input clip: sub-sampled format is not supported when chroma is processed, convert it to YUV444 or RGB first. "
                "For the best quality, RGB colorspace is recommended as input.");
            return 1;
        }
        if (rvi->format->subSamplingH || rvi->format->subSamplingW)
        {
            setError(out, "clip \"ref\": sub-sampled format is not supported when chroma is processed, convert it to YUV444 or RGB first. "
                "For the best quality, RGB colorspace is recommended as input.");
            return 1;
        }
    }

    return 0;
}


void VBM3D_Data_Base::init_filter_data()
{
    // Adjust sigma and thMSE to fit for the unnormalized YUV color space
    double normY, normU, normV;

    double Yr, Yg, Yb, Ur, Ug, Ub, Vr, Vg, Vb;
    ColorMatrix_RGB2YUV_Parameter(matrix, Yr, Yg, Yb, Ur, Ug, Ub, Vr, Vg, Vb);

    normY = sqrt(Yr * Yr + Yg * Yg + Yb * Yb);
    normU = sqrt(Ur * Ur + Ug * Ug + Ub * Ub);
    normV = sqrt(Vr * Vr + Vg * Vg + Vb * Vb);

    para.thMSE *= normY;

    // Initialize BM3D data - FFTW plans, unnormalized transform amplification factor, hard threshold table, etc.
    if (process[0]) f[0] = BM3D_FilterData(wiener, para.sigma[0] / double(255) * normY,
        para.GroupSize, para.BlockSize, para.lambda);
    if (process[1]) f[1] = BM3D_FilterData(wiener, para.sigma[1] / double(255) * normU,
        para.GroupSize, para.BlockSize, para.lambda);
    if (process[2]) f[2] = BM3D_FilterData(wiener, para.sigma[2] / double(255) * normV,
        para.GroupSize, para.BlockSize, para.lambda);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class VBM3D_Process_Base


void VBM3D_Process_Base::Kernel(const std::vector<FLType *> &dst,
    const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref) const
{
    std::vector<FLType *> ResNum(frames), ResDen(frames);

    for (int f = 0; f < frames; ++f)
    {
        ResNum[f] = dst[f * 2];
        ResDen[f] = dst[f * 2 + 1];

        LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], [&](PCType i)
        {
            ResNum[f][i] = 0;
            ResDen[f][i] = 0;
        });
    }

    const PCType BlockPosBottom = height - d.para.BlockSize;
    const PCType BlockPosRight = width - d.para.BlockSize;

    for (PCType j = 0;; j += d.para.BlockStep)
    {
        // Handle scan of reference block - vertical
        if (j >= BlockPosBottom + d.para.BlockStep)
        {
            break;
        }
        else if (j > BlockPosBottom)
        {
            j = BlockPosBottom;
        }

        for (PCType i = 0;; i += d.para.BlockStep)
        {
            // Handle scan of reference block - horizontal
            if (i >= BlockPosRight + d.para.BlockStep)
            {
                break;
            }
            else if (i > BlockPosRight)
            {
                i = BlockPosRight;
            }

            // Form a group by block matching between reference block and its spatial-temporal neighborhood in the reference planes
            Pos3PairCode matchCode = BlockMatching(ref, j, i);

            // Get the filtered result through collaborative filtering and aggregation of matched blocks
            CollaborativeFilter(0, ResNum, ResDen, src, ref, matchCode);
        }
    }
}


void VBM3D_Process_Base::Kernel(const std::vector<FLType *> &dstY, const std::vector<FLType *> &dstU, const std::vector<FLType *> &dstV,
    const std::vector<const FLType *> &srcY, const std::vector<const FLType *> &srcU, const std::vector<const FLType *> &srcV,
    const std::vector<const FLType *> &refY, const std::vector<const FLType *> &refU, const std::vector<const FLType *> &refV) const
{
    std::vector<FLType *> ResNumY(frames), ResDenY(frames);
    std::vector<FLType *> ResNumU(frames), ResDenU(frames);
    std::vector<FLType *> ResNumV(frames), ResDenV(frames);

    if (d.process[0]) for (int f = 0; f < frames; ++f)
    {
        ResNumY[f] = dstY[f * 2];
        ResDenY[f] = dstY[f * 2 + 1];

        LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], [&](PCType i)
        {
            ResNumY[f][i] = 0;
            ResDenY[f][i] = 0;
        });
    }

    if (d.process[1]) for (int f = 0; f < frames; ++f)
    {
        ResNumU[f] = dstU[f * 2];
        ResDenU[f] = dstU[f * 2 + 1];

        LOOP_VH(dst_height[1], dst_width[1], dst_stride[1], [&](PCType i)
        {
            ResNumU[f][i] = 0;
            ResDenU[f][i] = 0;
        });
    }

    if (d.process[2]) for (int f = 0; f < frames; ++f)
    {
        ResNumV[f] = dstV[f * 2];
        ResDenV[f] = dstV[f * 2 + 1];

        LOOP_VH(dst_height[2], dst_width[2], dst_stride[2], [&](PCType i)
        {
            ResNumV[f][i] = 0;
            ResDenV[f][i] = 0;
        });
    }

    const PCType BlockPosBottom = height - d.para.BlockSize;
    const PCType BlockPosRight = width - d.para.BlockSize;

    for (PCType j = 0;; j += d.para.BlockStep)
    {
        // Handle scan of reference block - vertical
        if (j >= BlockPosBottom + d.para.BlockStep)
        {
            break;
        }
        else if (j > BlockPosBottom)
        {
            j = BlockPosBottom;
        }

        for (PCType i = 0;; i += d.para.BlockStep)
        {
            // Handle scan of reference block - horizontal
            if (i >= BlockPosRight + d.para.BlockStep)
            {
                break;
            }
            else if (i > BlockPosRight)
            {
                i = BlockPosRight;
            }

            // Form a group by block matching between reference block and its spatial-temporal neighborhood in the reference planes
            Pos3PairCode matchCode = BlockMatching(refY, j, i);

            // Get the filtered result through collaborative filtering and aggregation of matched blocks
            if (d.process[0]) CollaborativeFilter(0, ResNumY, ResDenY, srcY, refY, matchCode);
            if (d.process[1]) CollaborativeFilter(1, ResNumU, ResDenU, srcU, refU, matchCode);
            if (d.process[2]) CollaborativeFilter(2, ResNumV, ResDenV, srcV, refV, matchCode);
        }
    }
}


VBM3D_Process_Base::Pos3PairCode VBM3D_Process_Base::BlockMatching(
    const std::vector<const FLType *> &ref, PCType j, PCType i) const
{
    // Skip block matching if GroupSize is 1 or thMSE is not positive,
    // and take the reference block as the only element in the group
    if (d.para.GroupSize == 1 || d.para.thMSE <= 0)
    {
        return Pos3PairCode(1, Pos3Pair(KeyType(0), Pos3Type(cur, j, i)));
    }

    int f;
    Pos3PairCode matchCode;
    PosPairCode frameMatch;
    PosCode prePosCode;

    // Get reference block from the reference plane in current frame
    block_type refBlock(ref[cur], ref_stride[0], d.para.BlockSize, d.para.BlockSize, PosType(j, i));

    // Block Matching in current frame
    f = cur;

    frameMatch = refBlock.BlockMatchingMulti(ref[f],
        ref_height[0], ref_width[0], ref_stride[0], FLType(1),
        d.para.BMrange, d.para.BMstep, d.para.thMSE, 1, d.para.GroupSize, true);

    matchCode.resize(matchCode.size() + frameMatch.size());
    std::transform(frameMatch.begin(), frameMatch.end(),
        matchCode.end() - frameMatch.size(), [&](PosPair x)
    {
        return Pos3Pair(x.first, Pos3Type(x.second, f));
    });

    PCType nextPosNum = Min(d.para.PSnum, static_cast<PCType>(frameMatch.size()));
    PosCode curPosCode(nextPosNum);
    std::transform(frameMatch.begin(), frameMatch.begin() + nextPosNum,
        curPosCode.begin(), [](PosPair x)
    {
        return x.second;
    });

    PosCode curSearchPos = refBlock.GenSearchPos(curPosCode,
        ref_height[0], ref_width[0], d.para.PSrange, d.para.PSstep);

    // Predictive Search Block Matching in backward frames
    f = cur - 1;

    for (; f >= 0; --f)
    {
        if (f == cur - 1)
        {
            frameMatch = refBlock.BlockMatchingMulti(ref[f], ref_stride[0], FLType(1),
                curSearchPos, d.para.thMSE, d.para.GroupSize, true);
        }
        else
        {
            PCType nextPosNum = Min(d.para.PSnum, static_cast<PCType>(frameMatch.size()));
            prePosCode.resize(nextPosNum);
            std::transform(frameMatch.begin(), frameMatch.begin() + nextPosNum,
                prePosCode.begin(), [](PosPair x)
            {
                return x.second;
            });

            PosCode searchPos = refBlock.GenSearchPos(prePosCode,
                ref_height[0], ref_width[0], d.para.PSrange, d.para.PSstep);
            frameMatch = refBlock.BlockMatchingMulti(ref[f], ref_stride[0], FLType(1),
                searchPos, d.para.thMSE, d.para.GroupSize, true);
        }

        matchCode.resize(matchCode.size() + frameMatch.size());
        std::transform(frameMatch.begin(), frameMatch.end(),
            matchCode.end() - frameMatch.size(), [&](PosPair x)
        {
            return Pos3Pair(x.first, Pos3Type(x.second, f));
        });
    }

    // Predictive Search Block Matching in forward frames
    f = cur + 1;

    for (; f < frames; ++f)
    {
        if (f == cur + 1)
        {
            frameMatch = refBlock.BlockMatchingMulti(ref[f], ref_stride[0], FLType(1),
                curSearchPos, d.para.thMSE, d.para.GroupSize, true);
        }
        else
        {
            PCType nextPosNum = Min(d.para.PSnum, static_cast<PCType>(frameMatch.size()));
            prePosCode.resize(nextPosNum);
            std::transform(frameMatch.begin(), frameMatch.begin() + nextPosNum,
                prePosCode.begin(), [](PosPair x)
            {
                return x.second;
            });

            PosCode searchPos = refBlock.GenSearchPos(prePosCode,
                ref_height[0], ref_width[0], d.para.PSrange, d.para.PSstep);
            frameMatch = refBlock.BlockMatchingMulti(ref[f], ref_stride[0], FLType(1),
                searchPos, d.para.thMSE, d.para.GroupSize, true);
        }

        matchCode.resize(matchCode.size() + frameMatch.size());
        std::transform(frameMatch.begin(), frameMatch.end(),
            matchCode.end() - frameMatch.size(), [&](PosPair x)
        {
            return Pos3Pair(x.first, Pos3Type(x.second, f));
        });
    }

    // Limit the number of matched code to GroupSize
    if (d.para.GroupSize > 0 && static_cast<PCType>(matchCode.size()) > d.para.GroupSize)
    {
        std::partial_sort(matchCode.begin(), matchCode.begin() + d.para.GroupSize, matchCode.end());
        matchCode.resize(d.para.GroupSize);
    }

    return matchCode;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
