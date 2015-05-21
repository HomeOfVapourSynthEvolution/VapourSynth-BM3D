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


#ifndef VBM3D_BASE_H_
#define VBM3D_BASE_H_


#include "BM3D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct VBM3D_Para
    : public BM3D_Para
{
    typedef VBM3D_Para _Myt;
    typedef BM3D_Para _Mybase;

    int radius;
    PCType PSnum;
    PCType PSrange;
    PCType PSstep;

    VBM3D_Para(bool _wiener, std::string _profile = "lc");
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VBM3D_Data_Base
    : public VSData
{
public:
    typedef VBM3D_Data_Base _Myt;
    typedef VSData _Mybase;
    typedef VBM3D_Para _Mypara;

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
    explicit VBM3D_Data_Base(bool _wiener,
        const VSAPI *_vsapi = nullptr, std::string _FunctionName = "VBase", std::string _NameSpace = "bm3d")
        : _Mybase(_vsapi, _FunctionName, _NameSpace),
        wiener(_wiener), para_default(_wiener), para(_wiener), f(VSMaxPlaneCount)
    {}

    VBM3D_Data_Base(const _Myt &right) = delete;

    VBM3D_Data_Base(_Myt &&right)
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

    virtual ~VBM3D_Data_Base() override
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


class VBM3D_Process_Base
    : public VSProcess
{
public:
    typedef VBM3D_Process_Base _Myt;
    typedef VSProcess _Mybase;
    typedef VBM3D_Data_Base _Mydata;

    typedef Block<FLType, FLType> block_type;
    typedef block_type::KeyType KeyType;
    typedef block_type::PosType PosType;
    typedef block_type::PosPair PosPair;
    typedef block_type::KeyCode KeyCode;
    typedef block_type::PosCode PosCode;
    typedef block_type::PosPairCode PosPairCode;

    typedef BlockGroup<FLType, FLType> block_group;
    typedef block_group::Pos3Type Pos3Type;
    typedef block_group::Pos3Pair Pos3Pair;
    typedef block_group::Pos3Code Pos3Code;
    typedef block_group::Pos3PairCode Pos3PairCode;

private:
    const _Mydata &d;

protected:
    int b_offset;
    int f_offset;
    int cur;
    int frames;

    std::vector<const VSFrameRef *> v_src;
    std::vector<const VSFrameRef *> v_ref;

    const VSFormat *rfi = nullptr;

    PCType dst_height[VSMaxPlaneCount];
    PCType dst_pcount[VSMaxPlaneCount];

    PCType ref_height[VSMaxPlaneCount];
    PCType ref_width[VSMaxPlaneCount];
    PCType ref_stride[VSMaxPlaneCount];
    PCType ref_pcount[VSMaxPlaneCount];

private:
    template < typename _Ty >
    void process_core();

    template < typename _Ty >
    void process_core_gray();

    template < typename _Ty >
    void process_core_yuv();

    template < typename _Ty >
    void process_core_rgb();

protected:
    virtual void process_core8() override { process_core<uint8_t>(); }
    virtual void process_core16() override { process_core<uint16_t>(); }
    virtual void process_coreS() override { process_core<float>(); }

public:
    VBM3D_Process_Base(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {
        int total_frames = d.vi->numFrames;
        int radius = d.para.radius;

        b_offset = -Min(n - 0, radius);
        f_offset = Min(total_frames - 1 - n, radius);
        cur = -b_offset;
        frames = f_offset - b_offset + 1;

        for (int o = b_offset; o <= f_offset; ++o)
        {
            if (o == 0)
            {
                v_src.push_back(src);
            }
            else
            {
                v_src.push_back(vsapi->getFrameFilter(n + o, d.node, frameCtx));
            }
        }

        if (d.rdef)
        {
            for (int o = b_offset; o <= f_offset; ++o)
            {
                v_ref.push_back(vsapi->getFrameFilter(n + o, d.rnode, frameCtx));
            }

            rfi = vsapi->getFrameFormat(v_ref[cur]);
        }
        else
        {
            v_ref = v_src;
            rfi = fi;
        }

        if (!skip)
        {
            for (int i = 0; i < PlaneCount; ++i)
            {
                ref_height[i] = vsapi->getFrameHeight(v_ref[cur], i);
                ref_width[i] = vsapi->getFrameWidth(v_ref[cur], i);
                ref_stride[i] = vsapi->getStride(v_ref[cur], i) / rfi->bytesPerSample;
                ref_pcount[i] = ref_height[i] * ref_stride[i];
            }
        }
    }

    virtual ~VBM3D_Process_Base() override
    {
        for (int i = 0; i < frames; ++i)
        {
            if (i != cur)
            {
                vsapi->freeFrame(v_src[i]);
            }
        }

        if (d.rdef)
        {
            for (int i = 0; i < frames; ++i)
            {
                vsapi->freeFrame(v_ref[i]);
            }
        }
    }

    static const VSFormat *NewFormat(const _Mydata &d, const VSFormat *f, VSCore *core, const VSAPI *vsapi)
    {
        return vsapi->registerFormat(d.vi->format->colorFamily == cmRGB ? cmYUV : d.vi->format->colorFamily,
            stFloat, 32, f->subSamplingW, f->subSamplingH, core);
    }

protected:
    virtual void NewFormat()
    {
        dfi = NewFormat(d, fi, core, vsapi);
    }

    virtual void NewFrame() override
    {
        // Get input frame properties
        int error;
        const VSMap *src_map = vsapi->getFramePropsRO(src);

        int64_t BM3D_OPP = vsapi->propGetInt(src_map, "BM3D_OPP", 0, &error);

        if (!error && BM3D_OPP == 1 && fi->colorFamily == cmYUV && d.matrix != ColorMatrix::OPP)
        {
            vsapi->setFilterError("bm3d.VBasic/bm3d.VFinal - warning: "
                "There's a frame property \"BM3D_OPP=1\" indicating opponent color space input. "
                "You should specify \"matrix=100\" in the filter's argument.", frameCtx);
        }

        // The output frame is a stack of intermediate float data
        _NewFrame(width, height * (d.para.radius * 2 + 1) * 2, false);

        // Set the internal used dst_height and dst_pcount to a single part of the stacked data for convenience
        for (int i = 0; i < PlaneCount; ++i)
        {
            dst_height[i] = height;
            dst_pcount[i] = dst_height[i] * dst_stride[i];
        }

        // Set output frame properties
        VSMap *dst_map = vsapi->getFramePropsRW(dst);

        if (fi->colorFamily == cmRGB)
        {
            vsapi->propSetInt(dst_map, "BM3D_OPP", 1, paReplace);
        }

        vsapi->propSetInt(dst_map, "BM3D_V_radius", d.para.radius, paReplace);

        int64_t process[VSMaxPlaneCount];

        for (int i = 0; i < VSMaxPlaneCount; i++)
        {
            process[i] = d.process[i];
        }

        vsapi->propSetIntArray(dst_map, "BM3D_V_process", process, VSMaxPlaneCount);
    }

    void Kernel(std::vector<FLType *> &dst, std::vector<const FLType *> &src, std::vector<const FLType *> &ref);

    void Kernel(std::vector<FLType *> &dstY, std::vector<FLType *> &dstU, std::vector<FLType *> &dstV,
        std::vector<const FLType *> &srcY, std::vector<const FLType *> &srcU, std::vector<const FLType *> &srcV,
        std::vector<const FLType *> &refY, std::vector<const FLType *> &refU, std::vector<const FLType *> &refV);

    Pos3PairCode BlockMatching(std::vector<const FLType *> &ref, PCType j, PCType i);

    virtual void CollaborativeFilter(int plane,
        std::vector<FLType *> &ResNum, std::vector<FLType *> &ResDen,
        std::vector<const FLType *> &src, std::vector<const FLType *> &ref,
        const Pos3PairCode &code) = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "VBM3D_Base.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
