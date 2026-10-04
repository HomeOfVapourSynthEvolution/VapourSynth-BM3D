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


#ifndef CONVERSION_HPP_
#define CONVERSION_HPP_


#include <type_traits>
#include "Helper.h"
#include "Specification.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Integer output is computed in double, float would absorb the 0.499999 PC chroma tie-break into the neutral value
template < typename _Dt1 >
using CalcType = std::conditional_t<isFloat(_Dt1), FLType, double>;


// clipFloor/clipCeil bound the clipped output, they can be wider than the nominal dFloor/dCeil used for scaling
template < typename _Dt1, typename _St1 >
void RangeConvert(_Dt1 *dst, const _St1 *src,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloor, _Dt1 dNeutral, _Dt1 dCeil,
    _St1 sFloor, _St1 sNeutral, _St1 sCeil,
    bool clip, _Dt1 clipFloor, _Dt1 clipCeil)
{
    typedef _St1 srcType;
    typedef _Dt1 dstType;
    typedef CalcType<dstType> calcType;

    const bool dstFloat = isFloat(dstType);

    const auto sRange = sCeil - sFloor;
    const auto dRange = dCeil - dFloor;

    bool dstPCChroma = isPCChroma(dFloor, dNeutral, dCeil);

    calcType gain = static_cast<calcType>(dRange) / sRange;
    calcType offset = -static_cast<calcType>(sNeutral) * gain + dNeutral;
    if (!dstFloat) offset += calcType(dstPCChroma ? 0.499999 : 0.5);

    if (clip)
    {
        const calcType lowerL = static_cast<calcType>(clipFloor);
        const calcType upperL = static_cast<calcType>(clipCeil);

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            dst[i0] = static_cast<dstType>(Clip(static_cast<calcType>(src[i1]) * gain + offset, lowerL, upperL));
        });
    }
    else
    {
        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            dst[i0] = static_cast<dstType>(static_cast<calcType>(src[i1]) * gain + offset);
        });
    }
}

template < typename _Dt1, typename _St1 >
void RangeConvert(_Dt1 *dst, const _St1 *src,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloor, _Dt1 dNeutral, _Dt1 dCeil,
    _St1 sFloor, _St1 sNeutral, _St1 sCeil,
    bool clip = false)
{
    RangeConvert(dst, src, height, width, dst_stride, src_stride, dFloor, dNeutral, dCeil, sFloor, sNeutral, sCeil, clip, dFloor, dCeil);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Dt1, typename _St1 >
void ConvertToY(_Dt1 *dst, const _St1 *srcR, const _St1 *srcG, const _St1 *srcB,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloor, _Dt1 dCeil, _St1 sFloor, _St1 sCeil,
    ColorMatrix matrix = ColorMatrix::OPP, bool clip = false)
{
    typedef _St1 srcType;
    typedef _Dt1 dstType;

    const bool dstFloat = isFloat(dstType);

    const auto sRange = sCeil - sFloor;
    const auto dRange = dCeil - dFloor;

    const FLType lowerL = static_cast<FLType>(dFloor);
    const FLType upperL = static_cast<FLType>(dCeil);

    if (matrix == ColorMatrix::GBR)
    {
        RangeConvert(dst, srcG, height, width, dst_stride, src_stride, dFloor, dFloor, dCeil, sFloor, sFloor, sCeil, false);
    }
    else if (matrix == ColorMatrix::OPP)
    {
        FLType gain = static_cast<FLType>(dRange) / (sRange * FLType(3));
        FLType offset = -static_cast<FLType>(sFloor) * FLType(3) * gain + dFloor;
        if (!dstFloat) offset += FLType(0.5);

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            FLType temp = (static_cast<FLType>(srcR[i1])
                + static_cast<FLType>(srcG[i1])
                + static_cast<FLType>(srcB[i1]))
                * gain + offset;
            dst[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
        });
    }
    else if (matrix == ColorMatrix::Minimum)
    {
        FLType gain = static_cast<FLType>(dRange) / sRange;
        FLType offset = -static_cast<FLType>(sFloor) * gain + dFloor;
        if (!dstFloat) offset += FLType(0.5);

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            FLType temp = static_cast<FLType>(
                ::Min(srcR[i1], ::Min(srcG[i1], srcB[i1]))
                ) * gain + offset;
            dst[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
        });
    }
    else if (matrix == ColorMatrix::Maximum)
    {
        FLType gain = static_cast<FLType>(dRange) / sRange;
        FLType offset = -static_cast<FLType>(sFloor) * gain + dFloor;
        if (!dstFloat) offset += FLType(0.5);

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            FLType temp = static_cast<FLType>(
                ::Max(srcR[i1], ::Max(srcG[i1], srcB[i1]))
                ) * gain + offset;
            dst[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
        });
    }
    else
    {
        FLType gain = static_cast<FLType>(dRange) / sRange;
        FLType offset = -static_cast<FLType>(sFloor) * gain + dFloor;
        if (!dstFloat) offset += FLType(0.5);

        FLType Kr, Kg, Kb;
        ColorMatrix_Parameter(matrix, Kr, Kg, Kb);

        Kr *= gain;
        Kg *= gain;
        Kb *= gain;

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            FLType temp = Kr * static_cast<FLType>(srcR[i1])
                + Kg * static_cast<FLType>(srcG[i1])
                + Kb * static_cast<FLType>(srcB[i1])
                + offset;
            dst[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
        });
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Dt1, typename _St1 >
void MatrixConvert_RGB2YUV(_Dt1 *dstY, _Dt1 *dstU, _Dt1 *dstV,
    const _St1 *srcR, const _St1 *srcG, const _St1 *srcB,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloorY, _Dt1 dCeilY, _Dt1 dFloorC, _Dt1 dNeutralC, _Dt1 dCeilC, _St1 sFloor, _St1 sCeil,
    ColorMatrix matrix = ColorMatrix::OPP, bool clip = false)
{
    typedef _St1 srcType;
    typedef _Dt1 dstType;
    typedef CalcType<dstType> calcType;

    const bool dstFloat = isFloat(dstType);

    const bool dstPCChroma = isPCChroma(dFloorC, dNeutralC, dCeilC);

    const auto sRange = sCeil - sFloor;
    const auto dRangeY = dCeilY - dFloorY;
    const auto dRangeC = dCeilC - dFloorC;

    const calcType lowerLY = static_cast<calcType>(dFloorY);
    const calcType upperLY = static_cast<calcType>(dCeilY);
    const calcType lowerLC = static_cast<calcType>(dFloorC);
    const calcType upperLC = static_cast<calcType>(dCeilC);

    if (matrix == ColorMatrix::GBR)
    {
        RangeConvert(dstY, srcG, height, width, dst_stride, src_stride, dFloorY, dFloorY, dCeilY, sFloor, sFloor, sCeil, clip);
        RangeConvert(dstU, srcB, height, width, dst_stride, src_stride, dFloorY, dFloorY, dCeilY, sFloor, sFloor, sCeil, clip);
        RangeConvert(dstV, srcR, height, width, dst_stride, src_stride, dFloorY, dFloorY, dCeilY, sFloor, sFloor, sCeil, clip);
    }
    else if (matrix == ColorMatrix::OPP)
    {
        calcType gainY = static_cast<calcType>(dRangeY) / (sRange * calcType(3));
        calcType offsetY = -static_cast<calcType>(sFloor) * calcType(3) * gainY + dFloorY;
        if (!dstFloat) offsetY += calcType(0.5);
        calcType gainU = static_cast<calcType>(dRangeC) / (sRange * calcType(2));
        calcType gainV = static_cast<calcType>(dRangeC) / (sRange * calcType(4));
        calcType offsetC = static_cast<calcType>(dNeutralC);
        if (!dstFloat) offsetC += calcType(dstPCChroma ? 0.499999 : 0.5);

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            calcType temp;

            temp = (static_cast<calcType>(srcR[i1])
                + static_cast<calcType>(srcG[i1])
                + static_cast<calcType>(srcB[i1]))
                * gainY + offsetY;
            dstY[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLY, upperLY) : temp);

            temp = (static_cast<calcType>(srcR[i1])
                - static_cast<calcType>(srcB[i1]))
                * gainU + offsetC;
            dstU[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLC, upperLC) : temp);

            temp = (static_cast<calcType>(srcR[i1])
                - static_cast<calcType>(srcG[i1]) * calcType(2)
                + static_cast<calcType>(srcB[i1]))
                * gainV + offsetC;
            dstV[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLC, upperLC) : temp);
        });
    }
    else if (matrix == ColorMatrix::Minimum || matrix == ColorMatrix::Maximum)
    {
        std::cerr << "MatrixConvert_RGB2YUV: ColorMatrix::Minimum or ColorMatrix::Maximum is invalid!\n";
        return;
    }
    else
    {
        calcType gainY = static_cast<calcType>(dRangeY) / sRange;
        calcType offsetY = -static_cast<calcType>(sFloor) * gainY + dFloorY;
        if (!dstFloat) offsetY += calcType(0.5);
        calcType gainC = static_cast<calcType>(dRangeC) / sRange;
        calcType offsetC = static_cast<calcType>(dNeutralC);
        if (!dstFloat) offsetC += calcType(dstPCChroma ? 0.499999 : 0.5);

        calcType Yr, Yg, Yb, Ur, Ug, Ub, Vr, Vg, Vb;
        ColorMatrix_RGB2YUV_Parameter(matrix, Yr, Yg, Yb, Ur, Ug, Ub, Vr, Vg, Vb);

        Yr *= gainY;
        Yg *= gainY;
        Yb *= gainY;
        Ur *= gainC;
        Ug *= gainC;
        Ub *= gainC;
        Vr *= gainC;
        Vg *= gainC;
        Vb *= gainC;

        LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
        {
            calcType temp;

            temp = Yr * static_cast<calcType>(srcR[i1])
                + Yg * static_cast<calcType>(srcG[i1])
                + Yb * static_cast<calcType>(srcB[i1])
                + offsetY;
            dstY[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLY, upperLY) : temp);

            temp = Ur * static_cast<calcType>(srcR[i1])
                + Ug * static_cast<calcType>(srcG[i1])
                + Ub * static_cast<calcType>(srcB[i1])
                + offsetC;
            dstU[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLC, upperLC) : temp);

            temp = Vr * static_cast<calcType>(srcR[i1])
                + Vg * static_cast<calcType>(srcG[i1])
                + Vb * static_cast<calcType>(srcB[i1])
                + offsetC;
            dstV[i0] = static_cast<dstType>(clip ? Clip(temp, lowerLC, upperLC) : temp);
        });
    }
}


template < typename _Dt1, typename _St1 >
void MatrixConvert_YUV2RGB(_Dt1 *dstR, _Dt1 *dstG, _Dt1 *dstB,
    const _St1 *srcY, const _St1 *srcU, const _St1 *srcV,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloor, _Dt1 dCeil, _St1 sFloorY, _St1 sCeilY, _St1 sFloorC, _St1 sNeutralC, _St1 sCeilC,
    ColorMatrix matrix, bool clip, _Dt1 clipFloor, _Dt1 clipCeil)
{
    typedef _St1 srcType;
    typedef _Dt1 dstType;

    const bool dstFloat = isFloat(dstType);

    const auto sRangeY = sCeilY - sFloorY;
    const auto sRangeC = sCeilC - sFloorC;
    const auto dRange = dCeil - dFloor;

    const FLType lowerL = static_cast<FLType>(clipFloor);
    const FLType upperL = static_cast<FLType>(clipCeil);

    if (matrix == ColorMatrix::GBR)
    {
        RangeConvert(dstG, srcY, height, width, dst_stride, src_stride, dFloor, dFloor, dCeil, sFloorY, sFloorY, sCeilY, clip, clipFloor, clipCeil);
        RangeConvert(dstB, srcU, height, width, dst_stride, src_stride, dFloor, dFloor, dCeil, sFloorY, sFloorY, sCeilY, clip, clipFloor, clipCeil);
        RangeConvert(dstR, srcV, height, width, dst_stride, src_stride, dFloor, dFloor, dCeil, sFloorY, sFloorY, sCeilY, clip, clipFloor, clipCeil);
    }
    else if (matrix == ColorMatrix::Minimum || matrix == ColorMatrix::Maximum)
    {
        std::cerr << "MatrixConvert_YUV2RGB: ColorMatrix::Minimum or ColorMatrix::Maximum is invalid!\n";
        return;
    }
    else
    {
        FLType gainY = static_cast<FLType>(dRange) / sRangeY;
        FLType gainC = static_cast<FLType>(dRange) / sRangeC;

        FLType Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv;
        ColorMatrix_YUV2RGB_Parameter(matrix, Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv);

        Ry *= gainY;
        Ru *= gainC;
        Rv *= gainC;
        Gy *= gainY;
        Gu *= gainC;
        Gv *= gainC;
        By *= gainY;
        Bu *= gainC;
        Bv *= gainC;

        FLType offsetR = -static_cast<FLType>(sFloorY) * Ry - sNeutralC * (Ru + Rv) + dFloor;
        if (!dstFloat) offsetR += FLType(0.5);
        FLType offsetG = -static_cast<FLType>(sFloorY) * Gy - sNeutralC * (Gu + Gv) + dFloor;
        if (!dstFloat) offsetG += FLType(0.5);
        FLType offsetB = -static_cast<FLType>(sFloorY) * By - sNeutralC * (Bu + Bv) + dFloor;
        if (!dstFloat) offsetB += FLType(0.5);

        if (matrix == ColorMatrix::OPP)
        {
            LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
            {
                FLType temp;

                temp = Ry * static_cast<FLType>(srcY[i1])
                    + Ru * static_cast<FLType>(srcU[i1])
                    + Rv * static_cast<FLType>(srcV[i1])
                    + offsetR;
                dstR[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);

                temp = Gy * static_cast<FLType>(srcY[i1])
                    + Gv * static_cast<FLType>(srcV[i1])
                    + offsetG;
                dstG[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);

                temp = By * static_cast<FLType>(srcY[i1])
                    + Bu * static_cast<FLType>(srcU[i1])
                    + Bv * static_cast<FLType>(srcV[i1])
                    + offsetB;
                dstB[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
            });
        }
        else
        {
            LOOP_VH(height, width, dst_stride, src_stride, [&](PCType i0, PCType i1)
            {
                FLType temp;

                temp = Ry * static_cast<FLType>(srcY[i1])
                    + Rv * static_cast<FLType>(srcV[i1])
                    + offsetR;
                dstR[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);

                temp = Gy * static_cast<FLType>(srcY[i1])
                    + Gu * static_cast<FLType>(srcU[i1])
                    + Gv * static_cast<FLType>(srcV[i1])
                    + offsetG;
                dstG[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);

                temp = By * static_cast<FLType>(srcY[i1])
                    + Bu * static_cast<FLType>(srcU[i1])
                    + offsetB;
                dstB[i0] = static_cast<dstType>(clip ? Clip(temp, lowerL, upperL) : temp);
            });
        }
    }
}

template < typename _Dt1, typename _St1 >
void MatrixConvert_YUV2RGB(_Dt1 *dstR, _Dt1 *dstG, _Dt1 *dstB,
    const _St1 *srcY, const _St1 *srcU, const _St1 *srcV,
    const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride,
    _Dt1 dFloor, _Dt1 dCeil, _St1 sFloorY, _St1 sCeilY, _St1 sFloorC, _St1 sNeutralC, _St1 sCeilC,
    ColorMatrix matrix = ColorMatrix::OPP, bool clip = false)
{
    MatrixConvert_YUV2RGB(dstR, dstG, dstB, srcY, srcU, srcV, height, width, dst_stride, src_stride,
        dFloor, dCeil, sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC, matrix, clip, dFloor, dCeil);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template functions of class VSProcess


template < typename _Ty >
void VSProcess::Int2Float(FLType *dst, const _Ty *src,
    PCType height, PCType width, PCType dst_stride, PCType src_stride,
    bool chroma, bool full, bool clip)
{
    FLType dFloor, dNeutral, dCeil;
    _Ty sFloor, sNeutral, sCeil;

    GetQuanPara(dFloor, dNeutral, dCeil, 32, true, chroma);
    GetQuanPara(sFloor, sNeutral, sCeil, fi->bitsPerSample, full, chroma);

    RangeConvert(dst, src, height, width, dst_stride, src_stride,
        dFloor, dNeutral, dCeil, sFloor, sNeutral, sCeil, clip);
}

template < typename _Ty >
void VSProcess::Float2Int(_Ty *dst, const FLType *src,
    PCType height, PCType width, PCType dst_stride, PCType src_stride,
    bool chroma, bool full, bool clip)
{
    _Ty dFloor, dNeutral, dCeil;
    FLType sFloor, sNeutral, sCeil;

    GetQuanPara(dFloor, dNeutral, dCeil, dfi->bitsPerSample, full, chroma);
    GetQuanPara(sFloor, sNeutral, sCeil, 32, true, chroma);

    // Clip to the whole code range so limited range output keeps its headroom and footroom
    _Ty clipFloor, clipCeil;
    GetQuanPara(clipFloor, clipCeil, dfi->bitsPerSample, true);

    RangeConvert(dst, src, height, width, dst_stride, src_stride,
        dFloor, dNeutral, dCeil, sFloor, sNeutral, sCeil, clip, clipFloor, clipCeil);
}

template < typename _Ty >
void VSProcess::RGB2FloatY(FLType *dst,
    const _Ty *srcR, const _Ty *srcG, const _Ty *srcB,
    PCType height, PCType width, PCType dst_stride, PCType src_stride,
    ColorMatrix matrix, bool full, bool clip)
{
    FLType dFloor, dCeil;
    _Ty sFloor, sCeil;

    GetQuanPara(dFloor, dCeil, 32, true);
    GetQuanPara(sFloor, sCeil, fi->bitsPerSample, full);

    ConvertToY(dst, srcR, srcG, srcB,
        height, width, dst_stride, src_stride,
        dFloor, dCeil, sFloor, sCeil,
        matrix, clip);
}

template < typename _Ty >
void VSProcess::RGB2FloatYUV(FLType *dstY, FLType *dstU, FLType *dstV,
    const _Ty *srcR, const _Ty *srcG, const _Ty *srcB,
    PCType height, PCType width, PCType dst_stride, PCType src_stride,
    ColorMatrix matrix, bool full, bool clip)
{
    FLType dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC;
    _Ty sFloor, sCeil;

    GetQuanPara(dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC, 32, true);
    GetQuanPara(sFloor, sCeil, fi->bitsPerSample, full);

    MatrixConvert_RGB2YUV(dstY, dstU, dstV, srcR, srcG, srcB,
        height, width, dst_stride, src_stride,
        dFloorY, dCeilY, dFloorC, dNeutralC, dCeilC, sFloor, sCeil,
        matrix, clip);
}

template < typename _Ty >
void VSProcess::FloatYUV2RGB(_Ty *dstR, _Ty *dstG, _Ty *dstB,
    const FLType *srcY, const FLType *srcU, const FLType *srcV,
    PCType height, PCType width, PCType dst_stride, PCType src_stride,
    ColorMatrix matrix, bool full, bool clip)
{
    _Ty dFloor, dCeil;
    FLType sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC;

    GetQuanPara(dFloor, dCeil, dfi->bitsPerSample, full);
    GetQuanPara(sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC, 32, true);

    // Clip to the whole code range so limited range output keeps its headroom and footroom
    _Ty clipFloor, clipCeil;
    GetQuanPara(clipFloor, clipCeil, dfi->bitsPerSample, true);

    MatrixConvert_YUV2RGB(dstR, dstG, dstB, srcY, srcU, srcV,
        height, width, dst_stride, src_stride,
        dFloor, dCeil, sFloorY, sCeilY, sFloorC, sNeutralC, sCeilC,
        matrix, clip, clipFloor, clipCeil);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
