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
    try
    {
        int error;
        int m;

        // input - clip
        node = vsapi->propGetNode(in, "input", 0, nullptr);
        vi = vsapi->getVideoInfo(node);

        if (!isConstantFormat(vi))
        {
            throw std::string("Invalid input clip, only constant format input supported");
        }
        if ((vi->format->sampleType == stInteger && vi->format->bitsPerSample > 16)
            || (vi->format->sampleType == stFloat && vi->format->bitsPerSample != 32))
        {
            throw std::string("Invalid input clip, only 8-16 bit integer or 32 bit float formats supported");
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
                throw std::string("Invalid clip \"ref\", only constant format input supported");
            }
            if (rvi->format != vi->format)
            {
                throw std::string("input clip and clip \"ref\" must be of the same format");
            }
            if (rvi->width != vi->width || rvi->height != vi->height)
            {
                throw std::string("input clip and clip \"ref\" must be of the same width and height");
            }
            if (rvi->numFrames != vi->numFrames)
            {
                throw std::string("input clip and clip \"ref\" must have the same number of frames");
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
            throw std::string("Unrecognized \"profile\" specified, should be \"fast\", \"lc\", \"np\", \"high\" or \"vn\"");
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
                    throw std::string("Invalid \"sigma\" assigned, must be a non-negative floating point number");
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
            throw std::string("Invalid \"radius\" assigned, must be an integer in [1, 16]");
        }

        // block_size - int
        para.BlockSize = int64ToIntS(vsapi->propGetInt(in, "block_size", 0, &error));

        if (error)
        {
            para.BlockSize = para_default.BlockSize;
        }
        else if (para.BlockSize < 1 || para.BlockSize > 64)
        {
            throw std::string("Invalid \"block_size\" assigned, must be an integer in [1, 64]");
        }
        else if (para.BlockSize > vi->width || para.BlockSize > vi->height)
        {
            throw std::string("Invalid \"block_size\" assigned, must not exceed width or height of the frame");
        }

        // block_step - int
        para.BlockStep = int64ToIntS(vsapi->propGetInt(in, "block_step", 0, &error));

        if (error)
        {
            para.BlockStep = para_default.BlockStep;
        }
        else if (para.BlockStep < 1 || para.BlockStep > para.BlockSize)
        {
            throw std::string("Invalid \"block_step\" assigned, must be an integer in [1, block_size]");
        }

        // group_size - int
        para.GroupSize = int64ToIntS(vsapi->propGetInt(in, "group_size", 0, &error));

        if (error)
        {
            para.GroupSize = para_default.GroupSize;
        }
        else if (para.GroupSize < 1 || para.GroupSize > 256)
        {
            throw std::string("Invalid \"group_size\" assigned, must be an integer in [1, 256]");
        }

        // bm_range - int
        para.BMrange = int64ToIntS(vsapi->propGetInt(in, "bm_range", 0, &error));

        if (error)
        {
            para.BMrange = para_default.BMrange;
        }
        else if (para.BMrange < 1)
        {
            throw std::string("Invalid \"bm_range\" assigned, must be a positive integer");
        }

        // bm_step - int
        para.BMstep = int64ToIntS(vsapi->propGetInt(in, "bm_step", 0, &error));

        if (error)
        {
            para.BMstep = para_default.BMstep;
        }
        else if (para.BMstep < 1 || para.BMstep > para.BMrange)
        {
            throw std::string("Invalid \"bm_step\" assigned, must be an integer in [1, bm_range]");
        }

        // ps_num - int
        para.PSnum = int64ToIntS(vsapi->propGetInt(in, "ps_num", 0, &error));

        if (error)
        {
            para.PSnum = para_default.PSnum;
        }
        else if (para.PSnum < 1 || para.PSnum > para.GroupSize)
        {
            throw std::string("Invalid \"ps_num\" assigned, must be an integer in [1, group_size]");
        }

        // ps_range - int
        para.PSrange = int64ToIntS(vsapi->propGetInt(in, "ps_range", 0, &error));

        if (error)
        {
            para.PSrange = para_default.PSrange;
        }
        else if (para.PSrange < 1)
        {
            throw std::string("Invalid \"ps_range\" assigned, must be a positive integer");
        }

        // ps_step - int
        para.PSstep = int64ToIntS(vsapi->propGetInt(in, "ps_step", 0, &error));

        if (error)
        {
            para.PSstep = para_default.PSstep;
        }
        else if (para.PSstep < 1 || para.PSstep > para.PSrange)
        {
            throw std::string("Invalid \"ps_step\" assigned, must be an integer in [1, ps_range]");
        }

        // th_mse - float
        para.thMSE = vsapi->propGetFloat(in, "th_mse", 0, &error);

        if (error)
        {
            para.thMSE_Default();
        }
        else if (para.thMSE <= 0)
        {
            throw std::string("Invalid \"th_mse\" assigned, must be a positive floating point number");
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
            throw std::string("Unsupported \"matrix\" specified");
        }

        // process
        for (int i = 0; i < VSMaxPlaneCount; i++)
        {
            if (vi->format->colorFamily != cmRGB && para.sigma[i] == 0)
            {
                process[i] = 0;
            }
        }

        if (process[1] || process[2])
        {
            if (vi->format->subSamplingH || vi->format->subSamplingW)
            {
                throw std::string("input clip: sub-sampled format is not supported when chroma is processed, convert it to YUV444 or RGB first. "
                    "For the best quality, RGB colorspace is recommended as input.");
            }
            if (rvi->format->subSamplingH || rvi->format->subSamplingW)
            {
                throw std::string("clip \"ref\": sub-sampled format is not supported when chroma is processed, convert it to YUV444 or RGB first. "
                    "For the best quality, RGB colorspace is recommended as input.");
            }
        }
    }
    catch (const std::string &error_msg)
    {
        setError(out, error_msg.c_str());
        return 1;
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
    const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref, const std::vector<const FLType *> &wref) const
{
    std::vector<FLType *> ResNum(frames), ResDen(frames);

    for (int f = 0; f < frames; ++f)
    {
        ResNum[f] = dst[f * 2];
        ResDen[f] = dst[f * 2 + 1];
    }

    memset(dst[0], 0, sizeof(FLType) * dst_pcount[0] * frames * 2);

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
            CollaborativeFilter(0, ResNum, ResDen, src, wref, matchCode);
        }
    }
}


void VBM3D_Process_Base::Kernel(const std::vector<FLType *> &dstY, const std::vector<FLType *> &dstU, const std::vector<FLType *> &dstV,
    const std::vector<const FLType *> &srcY, const std::vector<const FLType *> &srcU, const std::vector<const FLType *> &srcV,
    const std::vector<const FLType *> &refY,
    const std::vector<const FLType *> &wrefY, const std::vector<const FLType *> &wrefU, const std::vector<const FLType *> &wrefV) const
{
    std::vector<FLType *> ResNumY(frames), ResDenY(frames);
    std::vector<FLType *> ResNumU(frames), ResDenU(frames);
    std::vector<FLType *> ResNumV(frames), ResDenV(frames);

    if (d.process[0])
    {
        for (int f = 0; f < frames; ++f)
        {
            ResNumY[f] = dstY[f * 2];
            ResDenY[f] = dstY[f * 2 + 1];
        }

        memset(dstY[0], 0, sizeof(FLType) * dst_pcount[0] * frames * 2);
    }

    if (d.process[1])
    {
        for (int f = 0; f < frames; ++f)
        {
            ResNumU[f] = dstU[f * 2];
            ResDenU[f] = dstU[f * 2 + 1];
        }

        memset(dstU[0], 0, sizeof(FLType) * dst_pcount[1] * frames * 2);
    }

    if (d.process[2])
    {
        for (int f = 0; f < frames; ++f)
        {
            ResNumV[f] = dstV[f * 2];
            ResDenV[f] = dstV[f * 2 + 1];
        }

        memset(dstV[0], 0, sizeof(FLType) * dst_pcount[2] * frames * 2);
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
            if (d.process[0]) CollaborativeFilter(0, ResNumY, ResDenY, srcY, wrefY, matchCode);
            if (d.process[1]) CollaborativeFilter(1, ResNumU, ResDenU, srcU, wrefU, matchCode);
            if (d.process[2]) CollaborativeFilter(2, ResNumV, ResDenV, srcV, wrefV, matchCode);
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
        matchCode.end() - frameMatch.size(), [&](const PosPair &x)
    {
        return Pos3Pair(x.first, Pos3Type(x.second, f));
    });

    PCType nextPosNum = Min(d.para.PSnum, static_cast<PCType>(frameMatch.size()));
    PosCode curPosCode(nextPosNum);
    std::transform(frameMatch.begin(), frameMatch.begin() + nextPosNum,
        curPosCode.begin(), [](const PosPair &x)
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
                prePosCode.begin(), [](const PosPair &x)
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
            matchCode.end() - frameMatch.size(), [&](const PosPair &x)
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
                prePosCode.begin(), [](const PosPair &x)
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
            matchCode.end() - frameMatch.size(), [&](const PosPair &x)
        {
            return Pos3Pair(x.first, Pos3Type(x.second, f));
        });
    }

    // Limit the number of matched code to GroupSize
    if (d.para.GroupSize > 0 && static_cast<PCType>(matchCode.size()) > d.para.GroupSize)
    {
        std::partial_sort(matchCode.begin() + 1, matchCode.begin() + d.para.GroupSize, matchCode.end());
        matchCode.resize(d.para.GroupSize);
    }

    return matchCode;
}


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
    std::vector<const FLType *> wrefYv;

    std::vector<FLType *> srcYd(frames, nullptr), refYd(frames, nullptr), wrefYd(frames, nullptr);

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_src[i], 0));
        auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_ref[i], 0));

        auto wrefY = static_cast<const _Ty *>(nullptr);
        if (v_wref.size() > i)
            wrefY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 0));

        // Allocate memory for floating point Y data
        AlignedMalloc(srcYd[i], src_pcount[0]);
        if (d.rdef) AlignedMalloc(refYd[i], ref_pcount[0]);
        else refYd[i] = srcYd[i];
        if (d.wdef) AlignedMalloc(wrefYd[i], wref_pcount[0]);
        else wrefYd[i] = refYd[i];

        // Convert src and ref from integer Y data to floating point Y data
        Int2Float(srcYd[i], srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
        if (d.rdef) Int2Float(refYd[i], refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);
        if (d.wdef) Int2Float(wrefYd[i], wrefY, wref_height[0], wref_width[0], wref_stride[0], wref_stride[0], false, full, false);

        // Store pointer to floating point Y data into corresponding frame of the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        srcYv.push_back(srcYd[i]);
        refYv.push_back(refYd[i]);
        wrefYv.push_back(wrefYd[i]);
    }

    // Execute kernel
    Kernel(dstYv, srcYv, refYv, wrefYv);

    // Free memory for floating point Y data
    for (int i = 0; i < frames; ++i)
    {
        AlignedFree(srcYd[i]);
        if (d.rdef) AlignedFree(refYd[i]);
        if (d.wdef) AlignedFree(wrefYd[i]);
    }
}

template <>
void VBM3D_Process_Base::process_core_gray<FLType>()
{
    std::vector<FLType *> dstYv;
    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> refYv;
    std::vector<const FLType *> wrefYv;

    // Get write pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0))
        + dst_pcount[0] * 2 * (d.para.radius + b_offset);

    for (int i = 0; i < frames; ++i)
    {
        // Get read pointer
        auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_src[i], 0));
        auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_ref[i], 0));

        auto wrefY = static_cast<const FLType *>(nullptr);
        if (v_wref.size() > i)
            wrefY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_wref[i], 0));

        // Store pointer to floating point Y data into corresponding frame of the vector
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2));
        dstYv.push_back(dstY + dst_pcount[0] * (i * 2 + 1));
        srcYv.push_back(srcY);
        refYv.push_back(refY);
        wrefYv.push_back(wrefY);
    }

    // Execute kernel
    Kernel(dstYv, srcYv, refYv, wrefYv);
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

    std::vector<const FLType *> wrefYv;
    std::vector<const FLType *> wrefUv;
    std::vector<const FLType *> wrefVv;

    std::vector<FLType *> srcYd(frames, nullptr), srcUd(frames, nullptr), srcVd(frames, nullptr);
    std::vector<FLType *> refYd(frames, nullptr);
    std::vector<FLType *> wrefYd(frames, nullptr), wrefUd(frames, nullptr), wrefVd(frames, nullptr);

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

        auto wrefY = static_cast<const _Ty *>(nullptr);
        auto wrefU = static_cast<const _Ty *>(nullptr);
        auto wrefV = static_cast<const _Ty *>(nullptr);

        if (v_wref.size() > i) {
            wrefY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 0));
            wrefU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 1));
            wrefV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 2));
        }

        // Allocate memory for floating point YUV data
        if (d.process[0] || !d.rdef) AlignedMalloc(srcYd[i], src_pcount[0]);
        if (d.process[1]) AlignedMalloc(srcUd[i], src_pcount[1]);
        if (d.process[2]) AlignedMalloc(srcVd[i], src_pcount[2]);

        if (d.rdef)
            AlignedMalloc(refYd[i], ref_pcount[0]);
        else
            refYd[i] = srcYd[i];

        if (d.wdef && d.process[0])
            AlignedMalloc(wrefYd[i], wref_pcount[0]);
        else
            wrefYd[i] = refYd[i];
        if (d.wiener && d.process[1]) AlignedMalloc(wrefUd[i], wref_pcount[1]);
        if (d.wiener && d.process[2]) AlignedMalloc(wrefVd[i], wref_pcount[2]);

        // Convert src and ref from integer YUV data to floating point YUV data
        if (d.process[0] || !d.rdef) Int2Float(srcYd[i], srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
        if (d.process[1]) Int2Float(srcUd[i], srcU, src_height[1], src_width[1], src_stride[1], src_stride[1], true, full, false);
        if (d.process[2]) Int2Float(srcVd[i], srcV, src_height[2], src_width[2], src_stride[2], src_stride[2], true, full, false);

        if (d.rdef)
            Int2Float(refYd[i], refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);

        if (d.wdef) {
            if (d.process[0]) Int2Float(wrefYd[i], wrefY, wref_height[0], wref_width[0], wref_stride[0], wref_stride[0], false, full, false);
            if (d.process[1]) Int2Float(wrefUd[i], wrefU, wref_height[1], wref_width[1], wref_stride[1], wref_stride[1], true, full, false);
            if (d.process[2]) Int2Float(wrefVd[i], wrefV, wref_height[2], wref_width[2], wref_stride[2], wref_stride[2], true, full, false);
        }
        else if (d.wiener) {
            if (d.process[1]) Int2Float(wrefUd[i], refU, ref_height[1], ref_width[1], ref_stride[1], ref_stride[1], true, full, false);
            if (d.process[2]) Int2Float(wrefVd[i], refV, ref_height[2], ref_width[2], ref_stride[2], ref_stride[2], true, full, false);
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

        wrefYv.push_back(wrefYd[i]);
        wrefUv.push_back(wrefUd[i]);
        wrefVv.push_back(wrefVd[i]);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, wrefYv, wrefUv, wrefVv);

    // Free memory for floating point YUV data
    for (int i = 0; i < frames; ++i)
    {
        if (d.process[0] || !d.rdef) AlignedFree(srcYd[i]);
        if (d.process[1]) AlignedFree(srcUd[i]);
        if (d.process[2]) AlignedFree(srcVd[i]);

        if (d.rdef) AlignedFree(refYd[i]);

        if (d.wdef && d.process[0]) AlignedFree(wrefYd[i]);
        if (d.wiener && d.process[1]) AlignedFree(wrefUd[i]);
        if (d.wiener && d.process[2]) AlignedFree(wrefVd[i]);
    }
}

template <>
void VBM3D_Process_Base::process_core_yuv<FLType>()
{
    std::vector<FLType *> dstYv;
    std::vector<FLType *> dstUv;
    std::vector<FLType *> dstVv;

    std::vector<const FLType *> srcYv;
    std::vector<const FLType *> srcUv;
    std::vector<const FLType *> srcVv;

    std::vector<const FLType *> refYv;

    std::vector<const FLType *> wrefYv;
    std::vector<const FLType *> wrefUv;
    std::vector<const FLType *> wrefVv;

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

        auto wrefY = static_cast<const FLType *>(nullptr);
        auto wrefU = static_cast<const FLType *>(nullptr);
        auto wrefV = static_cast<const FLType *>(nullptr);

        if (v_wref.size() > i) {
            wrefY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_wref[i], 0));
            wrefU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_wref[i], 1));
            wrefV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(v_wref[i], 2));
        }

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

        wrefYv.push_back(wrefY);
        wrefUv.push_back(wrefU);
        wrefVv.push_back(wrefV);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, wrefYv, wrefUv, wrefVv);
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

    std::vector<const FLType *> wrefYv;
    std::vector<const FLType *> wrefUv;
    std::vector<const FLType *> wrefVv;

    std::vector<FLType *> srcYd(frames, nullptr), srcUd(frames, nullptr), srcVd(frames, nullptr);
    std::vector<FLType *> refYd(frames, nullptr);
    std::vector<FLType *> wrefYd(frames, nullptr), wrefUd(frames, nullptr), wrefVd(frames, nullptr);

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

        auto wrefR = static_cast<const _Ty *>(nullptr);
        auto wrefG = static_cast<const _Ty *>(nullptr);
        auto wrefB = static_cast<const _Ty *>(nullptr);

        if (v_wref.size() > i) {
            wrefR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 0));
            wrefG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 1));
            wrefB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(v_wref[i], 2));
        }

        // Allocate memory for floating point YUV data
        AlignedMalloc(srcYd[i], src_pcount[0]);
        AlignedMalloc(srcUd[i], src_pcount[1]);
        AlignedMalloc(srcVd[i], src_pcount[2]);

        if (d.rdef)
            AlignedMalloc(refYd[i], ref_pcount[0]);
        else
            refYd[i] = srcYd[i];

        if (d.wdef)
            AlignedMalloc(wrefYd[i], wref_pcount[0]);
        else
            wrefYd[i] = refYd[i];
        if (d.wiener) AlignedMalloc(wrefUd[i], wref_pcount[1]);
        if (d.wiener) AlignedMalloc(wrefVd[i], wref_pcount[2]);

        // Convert src and ref from RGB data to floating point YUV data
        RGB2FloatYUV(srcYd[i], srcUd[i], srcVd[i], srcR, srcG, srcB,
            src_height[0], src_width[0], src_stride[0], src_stride[0],
            ColorMatrix::OPP, true, false);

        if (d.rdef)
            RGB2FloatY(refYd[i], refR, refG, refB,
                ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                ColorMatrix::OPP, true, false);

        if (d.wdef)
            RGB2FloatYUV(wrefYd[i], wrefUd[i], wrefVd[i], wrefR, wrefG, wrefB,
                wref_height[0], wref_width[0], wref_stride[0], wref_stride[0],
                ColorMatrix::OPP, true, false);
        else if (d.wiener)
            RGB2FloatYUV(wrefYd[i], wrefUd[i], wrefVd[i], refR, refG, refB,
                ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
                ColorMatrix::OPP, true, false);

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

        wrefYv.push_back(wrefYd[i]);
        wrefUv.push_back(wrefUd[i]);
        wrefVv.push_back(wrefVd[i]);
    }

    // Execute kernel
    Kernel(dstYv, dstUv, dstVv, srcYv, srcUv, srcVv, refYv, wrefYv, wrefUv, wrefVv);

    // Free memory for floating point YUV data
    for (int i = 0; i < frames; ++i)
    {
        AlignedFree(srcYd[i]);
        AlignedFree(srcUd[i]);
        AlignedFree(srcVd[i]);

        if (d.rdef)
            AlignedFree(refYd[i]);

        if (d.wdef)
            AlignedFree(wrefYd[i]);
        if (d.wiener) AlignedFree(wrefUd[i]);
        if (d.wiener) AlignedFree(wrefVd[i]);
    }
}


void VBM3D_Process_Base::process_core8() { process_core<uint8_t>(); }
void VBM3D_Process_Base::process_core16() { process_core<uint16_t>(); }
void VBM3D_Process_Base::process_coreS() { process_core<float>(); }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
