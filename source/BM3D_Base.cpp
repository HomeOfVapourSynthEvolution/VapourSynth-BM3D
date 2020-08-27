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


#include "BM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of class BM3D_Data_Base


int BM3D_Data_Base::arguments_process(const VSMap *in, VSMap *out)
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


void BM3D_Data_Base::init_filter_data()
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
// Functions of class BM3D_Process_Base


void BM3D_Process_Base::Kernel(FLType* dst, const FLType* src, const FLType* ref, const FLType* wref) const
{
    std::thread::id threadId = std::this_thread::get_id();
    FLType* ResNum = dst, * ResDen = nullptr;

    if (!d.buffer0.count(threadId))
    {
        AlignedMalloc(ResDen, dst_pcount[0]);
        d.buffer0.emplace(threadId, ResDen);
    }
    else
    {
        ResDen = d.buffer0.at(threadId);
    }

    memset(ResNum, 0, sizeof(FLType) * dst_pcount[0]);
    memset(ResDen, 0, sizeof(FLType) * dst_pcount[0]);

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

            // Form a group by block matching between reference block and its spatial neighborhood in the reference plane
            PosPairCode matchCode = BlockMatching(ref, j, i);

            // Get the filtered result through collaborative filtering and aggregation of matched blocks
            CollaborativeFilter(0, ResNum, ResDen, src, wref, matchCode);
        }
    }

    // The filtered blocks are sumed and averaged to form the final filtered image
    LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], [&](PCType i)
        {
            dst[i] = ResNum[i] / ResDen[i];
        });
}


void BM3D_Process_Base::Kernel(FLType* dstY, FLType* dstU, FLType* dstV,
    const FLType* srcY, const FLType* srcU, const FLType* srcV,
    const FLType* refY,
    const FLType* wrefY, const FLType* wrefU, const FLType* wrefV) const
{
    std::thread::id threadId = std::this_thread::get_id();
    FLType* ResNumY = dstY, * ResDenY = nullptr;
    FLType* ResNumU = dstU, * ResDenU = nullptr;
    FLType* ResNumV = dstV, * ResDenV = nullptr;

    if (d.process[0])
    {
        if (!d.buffer0.count(threadId))
        {
            AlignedMalloc(ResDenY, dst_pcount[0]);
            d.buffer0.emplace(threadId, ResDenY);
        }
        else
        {
            ResDenY = d.buffer0.at(threadId);
        }

        memset(ResNumY, 0, sizeof(FLType) * dst_pcount[0]);
        memset(ResDenY, 0, sizeof(FLType) * dst_pcount[0]);
    }

    if (d.process[1])
    {
        if (!d.buffer1.count(threadId))
        {
            AlignedMalloc(ResDenU, dst_pcount[1]);
            d.buffer1.emplace(threadId, ResDenU);
        }
        else
        {
            ResDenU = d.buffer1.at(threadId);
        }

        memset(ResNumU, 0, sizeof(FLType) * dst_pcount[1]);
        memset(ResDenU, 0, sizeof(FLType) * dst_pcount[1]);
    }

    if (d.process[2])
    {
        if (!d.buffer2.count(threadId))
        {
            AlignedMalloc(ResDenV, dst_pcount[2]);
            d.buffer2.emplace(threadId, ResDenV);
        }
        else
        {
            ResDenV = d.buffer2.at(threadId);
        }

        memset(ResNumV, 0, sizeof(FLType) * dst_pcount[2]);
        memset(ResDenV, 0, sizeof(FLType) * dst_pcount[2]);
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

            // Form a group by block matching between reference block and its spatial neighborhood in the reference plane
            PosPairCode matchCode = BlockMatching(refY, j, i);

            // Get the filtered result through collaborative filtering and aggregation of matched blocks
            if (d.process[0]) CollaborativeFilter(0, ResNumY, ResDenY, srcY, wrefY, matchCode);
            if (d.process[1]) CollaborativeFilter(1, ResNumU, ResDenU, srcU, wrefU, matchCode);
            if (d.process[2]) CollaborativeFilter(2, ResNumV, ResDenV, srcV, wrefV, matchCode);
        }
    }

    // The filtered blocks are sumed and averaged to form the final filtered image
    if (d.process[0]) LOOP_VH(dst_height[0], dst_width[0], dst_stride[0], [&](PCType i)
        {
            dstY[i] = ResNumY[i] / ResDenY[i];
        });

    if (d.process[1]) LOOP_VH(dst_height[1], dst_width[1], dst_stride[1], [&](PCType i)
        {
            dstU[i] = ResNumU[i] / ResDenU[i];
        });

    if (d.process[2]) LOOP_VH(dst_height[2], dst_width[2], dst_stride[2], [&](PCType i)
        {
            dstV[i] = ResNumV[i] / ResDenV[i];
        });
}


BM3D_Process_Base::PosPairCode BM3D_Process_Base::BlockMatching(
    const FLType *ref, PCType j, PCType i) const
{
    // Skip block matching if GroupSize is 1 or thMSE is not positive,
    // and take the reference block as the only element in the group
    if (d.para.GroupSize == 1 || d.para.thMSE <= 0)
    {
        return PosPairCode(1, PosPair(KeyType(0), PosType(j, i)));
    }
    
    // Get reference block from the reference plane
    block_type refBlock(ref, ref_stride[0], d.para.BlockSize, d.para.BlockSize, PosType(j, i));

    // Block matching
    return refBlock.BlockMatchingMulti(ref,
        ref_height[0], ref_width[0], ref_stride[0], FLType(1),
        d.para.BMrange, d.para.BMstep, d.para.thMSE, 1, d.para.GroupSize, true);
}


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
    FLType *dstYd = nullptr, *srcYd = nullptr, *refYd = nullptr, *wrefYd = nullptr;

    // Get write/read pointer
    auto dstY = reinterpret_cast<_Ty *>(vsapi->getWritePtr(dst, 0));
    auto srcY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(src, 0));
    auto refY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(ref, 0));

    auto wrefY = static_cast<const _Ty *>(nullptr);
    if (wref != nullptr)
        wrefY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 0));

    // Allocate memory for floating point Y data
    AlignedMalloc(dstYd, dst_pcount[0]);
    AlignedMalloc(srcYd, src_pcount[0]);
    if (d.rdef) AlignedMalloc(refYd, ref_pcount[0]);
    else refYd = srcYd;
    if (d.wdef) AlignedMalloc(wrefYd, wref_pcount[0]);
    else wrefYd = refYd;

    // Convert src and ref from integer Y data to floating point Y data
    Int2Float(srcYd, srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
    if (d.rdef) Int2Float(refYd, refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);
    if (d.wdef) Int2Float(wrefYd, wrefY, wref_height[0], wref_width[0], wref_stride[0], wref_stride[0], false, full, false);

    // Execute kernel
    Kernel(dstYd, srcYd, refYd, wrefYd);

    // Convert dst from floating point Y data to integer Y data
    Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, full, !isFloat(_Ty));

    // Free memory for floating point Y data
    AlignedFree(dstYd);
    AlignedFree(srcYd);
    if (d.rdef) AlignedFree(refYd);
    if (d.wdef) AlignedFree(wrefYd);
}

template <>
void BM3D_Process_Base::process_core_gray<FLType>()
{
    // Get write/read pointer
    auto dstY = reinterpret_cast<FLType *>(vsapi->getWritePtr(dst, 0));
    auto srcY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(src, 0));
    auto refY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(ref, 0));

    auto wrefY = static_cast<const FLType *>(nullptr);
    if (wref != nullptr)
        wrefY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(wref, 0));

    // Execute kernel
    Kernel(dstY, srcY, refY, wrefY);
}


template < typename _Ty >
void BM3D_Process_Base::process_core_yuv()
{
    FLType *dstYd = nullptr, *dstUd = nullptr, *dstVd = nullptr;
    FLType *srcYd = nullptr, *srcUd = nullptr, *srcVd = nullptr;
    FLType *refYd = nullptr;
    FLType *wrefYd = nullptr, *wrefUd = nullptr, *wrefVd = nullptr;

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

    auto wrefY = static_cast<const _Ty *>(nullptr);
    auto wrefU = static_cast<const _Ty *>(nullptr);
    auto wrefV = static_cast<const _Ty *>(nullptr);

    if (wref != nullptr) {
        wrefY = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 0));
        wrefU = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 1));
        wrefV = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 2));
    }

    // Allocate memory for floating point YUV data
    if (d.process[0]) AlignedMalloc(dstYd, dst_pcount[0]);
    if (d.process[1]) AlignedMalloc(dstUd, dst_pcount[1]);
    if (d.process[2]) AlignedMalloc(dstVd, dst_pcount[2]);

    if (d.process[0] || !d.rdef) AlignedMalloc(srcYd, src_pcount[0]);
    if (d.process[1]) AlignedMalloc(srcUd, src_pcount[1]);
    if (d.process[2]) AlignedMalloc(srcVd, src_pcount[2]);

    if (d.rdef)
        AlignedMalloc(refYd, ref_pcount[0]);
    else
        refYd = srcYd;

    if (d.wdef && d.process[0])
        AlignedMalloc(wrefYd, wref_pcount[0]);
    else
        wrefYd = refYd;
    if (d.wiener && d.process[1]) AlignedMalloc(wrefUd, wref_pcount[1]);
    if (d.wiener && d.process[2]) AlignedMalloc(wrefVd, wref_pcount[2]);

    // Convert src and ref from integer YUV data to floating point YUV data
    if (d.process[0] || !d.rdef) Int2Float(srcYd, srcY, src_height[0], src_width[0], src_stride[0], src_stride[0], false, full, false);
    if (d.process[1]) Int2Float(srcUd, srcU, src_height[1], src_width[1], src_stride[1], src_stride[1], true, full, false);
    if (d.process[2]) Int2Float(srcVd, srcV, src_height[2], src_width[2], src_stride[2], src_stride[2], true, full, false);

    if (d.rdef)
        Int2Float(refYd, refY, ref_height[0], ref_width[0], ref_stride[0], ref_stride[0], false, full, false);
    
    if (d.wdef) {
        if (d.process[0]) Int2Float(wrefYd, wrefY, wref_height[0], wref_width[0], wref_stride[0], wref_stride[0], false, full, false);
        if (d.process[1]) Int2Float(wrefUd, wrefU, wref_height[1], wref_width[1], wref_stride[1], wref_stride[1], true, full, false);
        if (d.process[2]) Int2Float(wrefVd, wrefV, wref_height[2], wref_width[2], wref_stride[2], wref_stride[2], true, full, false);
    }
    else if (d.wiener) {
        if (d.process[1]) Int2Float(wrefUd, refU, ref_height[1], ref_width[1], ref_stride[1], ref_stride[1], true, full, false);
        if (d.process[2]) Int2Float(wrefVd, refV, ref_height[2], ref_width[2], ref_stride[2], ref_stride[2], true, full, false);
    }

    // Execute kernel
    Kernel(dstYd, dstUd, dstVd, srcYd, srcUd, srcVd, refYd, wrefYd, wrefUd, wrefVd);

    // Convert dst from floating point YUV data to integer YUV data
    if (d.process[0]) Float2Int(dstY, dstYd, dst_height[0], dst_width[0], dst_stride[0], dst_stride[0], false, full, !isFloat(_Ty));
    if (d.process[1]) Float2Int(dstU, dstUd, dst_height[1], dst_width[1], dst_stride[1], dst_stride[1], true, full, !isFloat(_Ty));
    if (d.process[2]) Float2Int(dstV, dstVd, dst_height[2], dst_width[2], dst_stride[2], dst_stride[2], true, full, !isFloat(_Ty));

    // Free memory for floating point YUV data
    if (d.process[0]) AlignedFree(dstYd);
    if (d.process[1]) AlignedFree(dstUd);
    if (d.process[2]) AlignedFree(dstVd);

    if (d.process[0] || !d.rdef) AlignedFree(srcYd);
    if (d.process[1]) AlignedFree(srcUd);
    if (d.process[2]) AlignedFree(srcVd);

    if (d.rdef) AlignedFree(refYd);

    if (d.wdef && d.process[0]) AlignedFree(wrefYd);
    if (d.wiener && d.process[1]) AlignedFree(wrefUd);
    if (d.wiener && d.process[2]) AlignedFree(wrefVd);
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

    auto wrefY = static_cast<const FLType *>(nullptr);
    auto wrefU = static_cast<const FLType *>(nullptr);
    auto wrefV = static_cast<const FLType *>(nullptr);

    if (wref != nullptr) {
        wrefY = reinterpret_cast<const FLType *>(vsapi->getReadPtr(wref, 0));
        wrefU = reinterpret_cast<const FLType *>(vsapi->getReadPtr(wref, 1));
        wrefV = reinterpret_cast<const FLType *>(vsapi->getReadPtr(wref, 2));
    }

    // Execute kernel
    Kernel(dstY, dstU, dstV, srcY, srcU, srcV, refY, wrefY, wrefU, wrefV);
}


template < typename _Ty >
void BM3D_Process_Base::process_core_rgb()
{
    FLType *dstYd = nullptr, *dstUd = nullptr, *dstVd = nullptr;
    FLType *srcYd = nullptr, *srcUd = nullptr, *srcVd = nullptr;
    FLType *refYd = nullptr;
    FLType *wrefYd = nullptr, *wrefUd = nullptr, *wrefVd = nullptr;

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

    auto wrefR = static_cast<const _Ty *>(nullptr);
    auto wrefG = static_cast<const _Ty *>(nullptr);
    auto wrefB = static_cast<const _Ty *>(nullptr);

    if (wref != nullptr) {
        wrefR = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 0));
        wrefG = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 1));
        wrefB = reinterpret_cast<const _Ty *>(vsapi->getReadPtr(wref, 2));
    }

    // Allocate memory for floating point YUV data
    AlignedMalloc(dstYd, dst_pcount[0]);
    AlignedMalloc(dstUd, dst_pcount[1]);
    AlignedMalloc(dstVd, dst_pcount[2]);

    AlignedMalloc(srcYd, src_pcount[0]);
    AlignedMalloc(srcUd, src_pcount[1]);
    AlignedMalloc(srcVd, src_pcount[2]);

    if (d.rdef)
        AlignedMalloc(refYd, ref_pcount[0]);
    else
        refYd = srcYd;

    if (d.wdef)
        AlignedMalloc(wrefYd, wref_pcount[0]);
    else
        wrefYd = refYd;
    if (d.wiener) AlignedMalloc(wrefUd, wref_pcount[1]);
    if (d.wiener) AlignedMalloc(wrefVd, wref_pcount[2]);

    // Convert src and ref from RGB data to floating point YUV data
    RGB2FloatYUV(srcYd, srcUd, srcVd, srcR, srcG, srcB,
        src_height[0], src_width[0], src_stride[0], src_stride[0],
        ColorMatrix::OPP, true, false);

    if (d.rdef)
        RGB2FloatY(refYd, refR, refG, refB,
            ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
            ColorMatrix::OPP, true, false);
        
    if (d.wdef)
        RGB2FloatYUV(wrefYd, wrefUd, wrefVd, wrefR, wrefG, wrefB,
            wref_height[0], wref_width[0], wref_stride[0], wref_stride[0],
            ColorMatrix::OPP, true, false);
    else if (d.wiener)
        RGB2FloatYUV(wrefYd, wrefUd, wrefVd, refR, refG, refB,
            ref_height[0], ref_width[0], ref_stride[0], ref_stride[0],
            ColorMatrix::OPP, true, false);

    // Execute kernel
    Kernel(dstYd, dstUd, dstVd, srcYd, srcUd, srcVd, refYd, wrefYd, wrefUd, wrefVd);

    // Convert dst from floating point YUV data to RGB data
    FloatYUV2RGB(dstR, dstG, dstB, dstYd, dstUd, dstVd,
        dst_height[0], dst_width[0], dst_stride[0], dst_stride[0],
        ColorMatrix::OPP, true, !isFloat(_Ty));

    // Free memory for floating point YUV data
    AlignedFree(dstYd);
    AlignedFree(dstUd);
    AlignedFree(dstVd);

    AlignedFree(srcYd);
    AlignedFree(srcUd);
    AlignedFree(srcVd);

    if (d.rdef)
        AlignedFree(refYd);

    if (d.wdef)
        AlignedFree(wrefYd);
    if (d.wiener) AlignedFree(wrefUd);
    if (d.wiener) AlignedFree(wrefVd);
}


void BM3D_Process_Base::process_core8() { process_core<uint8_t>(); }
void BM3D_Process_Base::process_core16() { process_core<uint16_t>(); }
void BM3D_Process_Base::process_coreS() { process_core<float>(); }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
