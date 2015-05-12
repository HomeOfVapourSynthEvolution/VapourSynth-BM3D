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


#ifndef BM3D_BASE_H_
#define BM3D_BASE_H_


#include "fftw3_helper.hpp"
#include "Helper.h"
#include "Conversion.hpp"
#include "Block.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const struct BM3D_Para_Base
{
    std::string profile;
    std::vector<double> sigma;
    PCType BlockSize;
    PCType BlockStep;
    PCType GroupSize;
    PCType BMrange;
    PCType BMstep;
    double thMSE;
    double lambda;

    BM3D_Para_Base(std::string _profile = "lc")
        : profile(_profile), sigma({ 10.0, 10.0, 10.0 })
    {
        BMrange = 16;
        BMstep = 1;

        if (profile == "lc")
        {
            BMrange = 9;
        }
    }

    virtual void thMSE_Default() {}
} BM3D_Base_Default;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct BM3D_FilterData
{
    typedef BM3D_FilterData _Myt;

    typedef fftwh<FLType> fftw;

    std::vector<fftw::plan> fp;
    std::vector<fftw::plan> bp;
    std::vector<double> finalAMP;
    std::vector<std::vector<FLType>> thrTable;
    std::vector<FLType> wienerSigmaSqr;

    _Myt(bool wiener, double sigma, PCType GroupSize, PCType BlockSize, double lambda);

    _Myt(const _Myt &right) = delete;

    _Myt(_Myt &&right)
        : fp(std::move(right.fp)), bp(std::move(right.bp)),
        finalAMP(std::move(right.finalAMP)), thrTable(std::move(right.thrTable)),
        wienerSigmaSqr(std::move(right.wienerSigmaSqr))
    {}

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        fp = std::move(right.fp);
        bp = std::move(right.bp);
        finalAMP = std::move(right.finalAMP);
        thrTable = std::move(right.thrTable);
        wienerSigmaSqr = std::move(right.wienerSigmaSqr);

        return *this;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Data_Base
    : public VSData
{
public:
    typedef BM3D_Data_Base _Myt;
    typedef VSData _Mybase;
    typedef BM3D_Para_Base _Mypara;

public:
    bool rdef = false;
    VSNodeRef *rnode = nullptr;
    const VSVideoInfo *rvi = nullptr;

    _Mypara para_default;
    _Mypara para;

    bool wiener;
    ColorMatrix matrix;
    std::vector<BM3D_FilterData> f;

public:
    _Myt(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "Base", std::string _NameSpace = "bm3d",
        const _Mypara &_para = BM3D_Base_Default, bool _wiener = false)
        : _Mybase(_vsapi, _FunctionName, _NameSpace), para_default(_para), para(_para), wiener(_wiener), f()
    {}

    _Myt(const _Myt &right) = delete;

    _Myt(_Myt &&right)
        : _Mybase(std::move(right)),
        rdef(right.rdef), rnode(right.rnode), rvi(right.rvi),
        para_default(right.para_default), para(right.para),
        f(std::move(right.f))
    {
        right.rdef = false;
        right.rnode = nullptr;
        right.rvi = nullptr;
    }

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        _Mybase::operator=(std::move(right));

        rdef = right.rdef;
        rnode = right.rnode;
        rvi = right.rvi;

        para_default = right.para_default;
        para = right.para;

        f = std::move(right.f);

        right.rdef = false;
        right.rnode = nullptr;
        right.rvi = nullptr;

        return *this;
    }

    virtual ~BM3D_Data_Base() override
    {
        if (rdef && rnode) vsapi->freeNode(rnode);
    }

    virtual int arguments_process(const VSMap *in, VSMap *out) override;

protected:
    virtual void get_default_para(std::string _profile = "lc")
    {
        para_default = _Mypara(_profile);
    }

    void init_filter_data();
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Process_Base
    : public VSProcess
{
public:
    typedef BM3D_Process_Base _Myt;
    typedef VSProcess _Mybase;
    typedef BM3D_Data_Base _Mydata;

    typedef Block<FLType, FLType> block_type;
    typedef block_type::KeyType KeyType;
    typedef block_type::PosType PosType;
    typedef block_type::PosPair PosPair;
    typedef block_type::PosPairCode PosPairCode;
    typedef block_type::KeyCode KeyCode;
    typedef block_type::PosCode PosCode;

private:
    const _Mydata &d;

protected:
    const VSFrameRef *ref = nullptr;
    const VSFormat *rfi = nullptr;

    int ref_height[VSMaxPlaneCount];
    int ref_width[VSMaxPlaneCount];
    int ref_stride[VSMaxPlaneCount];
    int ref_pcount[VSMaxPlaneCount];

private:
    template < typename T >
    void process_core();

protected:
    virtual void process_core8() override { process_core<uint8_t>(); }
    virtual void process_core16() override { process_core<uint16_t>(); }

public:
    _Myt(const _Mydata &_d, int n, VSFrameContext *frameCtx, VSCore *core, const VSAPI *_vsapi)
        : _Mybase(_d, n, frameCtx, core, _vsapi), d(_d)
    {
        if (d.rdef)
        {
            ref = vsapi->getFrameFilter(n, d.rnode, frameCtx);
            rfi = vsapi->getFrameFormat(ref);
        }
        else
        {
            ref = src;
            rfi = fi;
        }

        if (!skip)
        {
            for (int i = 0; i < PlaneCount; ++i)
            {
                ref_height[i] = vsapi->getFrameHeight(ref, i);
                ref_width[i] = vsapi->getFrameWidth(ref, i);
                ref_stride[i] = vsapi->getStride(ref, i) / Bps;
                ref_pcount[i] = ref_height[i] * ref_stride[i];
            }
        }
    }

    virtual ~BM3D_Process_Base() override
    {
        if(d.rdef) vsapi->freeFrame(ref);
    }

    void Kernel(FLType *dst, const FLType *src, const FLType *ref);

    void Kernel(FLType *dstY, FLType *dstU, FLType *dstV,
        const FLType *srcY, const FLType *srcU, const FLType *srcV,
        const FLType *refY, const FLType *refU, const FLType *refV);

protected:
    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &posPairCode) = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "BM3D_Base.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
