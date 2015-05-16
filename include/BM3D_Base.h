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


#include "BM3D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Data_Base
    : public VSData
{
public:
    typedef BM3D_Data_Base _Myt;
    typedef VSData _Mybase;
    typedef BM3D_Para _Mypara;

public:
    bool rdef = false;
    VSNodeRef *rnode = nullptr;
    const VSVideoInfo *rvi = nullptr;

    bool wiener;
    ColorMatrix matrix;

    _Mypara para_default;
    _Mypara para;
    std::vector<BM3D_FilterData> f;

public:
    explicit BM3D_Data_Base(bool _wiener,
        const VSAPI *_vsapi = nullptr, std::string _FunctionName = "Base", std::string _NameSpace = "bm3d")
        : _Mybase(_vsapi, _FunctionName, _NameSpace), wiener(_wiener), para_default(_wiener), para(_wiener), f()
    {}

    BM3D_Data_Base(const _Myt &right) = delete;

    BM3D_Data_Base(_Myt &&right)
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

        if (rdef && rnode) vsapi->freeNode(rnode);

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
    void get_default_para(std::string _profile = "lc")
    {
        para_default = _Mypara(wiener, _profile);
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
    typedef block_type::KeyCode KeyCode;
    typedef block_type::PosCode PosCode;
    typedef block_type::PosPairCode PosPairCode;

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
    template < typename _Ty >
    void process_core();

    template < typename _Ty >
    void process_core_gray();

    template <>
    void process_core_gray<FLType>();

    template < typename _Ty >
    void process_core_yuv();

    template <>
    void process_core_yuv<FLType>();

    template < typename _Ty >
    void process_core_rgb();

protected:
    virtual void process_core8() override { process_core<uint8_t>(); }
    virtual void process_core16() override { process_core<uint16_t>(); }
    virtual void process_coreS() override { process_core<float>(); }

public:
    BM3D_Process_Base(const _Mydata &_d, int n, VSFrameContext *frameCtx, VSCore *core, const VSAPI *_vsapi)
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
                ref_stride[i] = vsapi->getStride(ref, i) / rfi->bytesPerSample;
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
    PosPairCode BlockMatching(const FLType *ref, PCType j, PCType i);

    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &posPairCode) = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "BM3D_Base.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
