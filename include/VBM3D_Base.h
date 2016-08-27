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

    VBM3D_Para(bool _wiener, std::string _profile = "fast");
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
    VBM3D_Data_Base(_Myt &&right) = delete;
    _Myt &operator=(const _Myt &right) = delete;
    _Myt &operator=(_Myt &&right) = delete;

    virtual ~VBM3D_Data_Base() override
    {
        if (rdef && rnode) vsapi->freeNode(rnode);
    }

    virtual int arguments_process(const VSMap *in, VSMap *out) override;

protected:
    void get_default_para(std::string _profile = "fast")
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

    bool full = true;

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
    virtual void process_core8() override;
    virtual void process_core16() override;
    virtual void process_coreS() override;

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
    virtual void NewFormat() override
    {
        dfi = NewFormat(d, fi, core, vsapi);
    }

    virtual void NewFrame() override
    {
        // Get input frame properties
        int error;
        const VSMap *src_map = vsapi->getFramePropsRO(src);

        // Determine OPP input
        int64_t BM3D_OPP = vsapi->propGetInt(src_map, "BM3D_OPP", 0, &error);

        if (error)
        {
            BM3D_OPP = 0;
        }
        else if (BM3D_OPP == 1 && fi->colorFamily != cmRGB && d.matrix != ColorMatrix::OPP)
        {
            vsapi->logMessage(mtWarning, "bm3d.VBasic/bm3d.VFinal - warning: "
                "There's a frame property \"BM3D_OPP=1\" indicating opponent color space input. "
                "You should specify \"matrix=100\" in the filter's argument.");
        }

        // Determine color range of Gray/YUV/YCoCg input
        int64_t _ColorRange = vsapi->propGetInt(src_map, "_ColorRange", 0, &error);

        if (error || BM3D_OPP == 1)
        {
            full = true;
        }
        else
        {
            full = _ColorRange != 1;
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

    void Kernel(const std::vector<FLType *> &dst, const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref) const;

    void Kernel(const std::vector<FLType *> &dstY, const std::vector<FLType *> &dstU, const std::vector<FLType *> &dstV,
        const std::vector<const FLType *> &srcY, const std::vector<const FLType *> &srcU, const std::vector<const FLType *> &srcV,
        const std::vector<const FLType *> &refY, const std::vector<const FLType *> &refU, const std::vector<const FLType *> &refV) const;

    Pos3PairCode BlockMatching(const std::vector<const FLType *> &ref, PCType j, PCType i) const;

    virtual void CollaborativeFilter(int plane,
        const std::vector<FLType *> &ResNum, const std::vector<FLType *> &ResDen,
        const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref,
        const Pos3PairCode &code) const = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
