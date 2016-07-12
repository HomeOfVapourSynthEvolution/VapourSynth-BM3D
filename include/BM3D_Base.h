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
        : _Mybase(_vsapi, _FunctionName, _NameSpace),
        wiener(_wiener), para_default(_wiener), para(_wiener), f(VSMaxPlaneCount)
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
    void get_default_para(std::string _profile = "fast")
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

    typedef BlockGroup<FLType, FLType> block_group;

private:
    const _Mydata &d;

protected:
    const VSFrameRef *ref = nullptr;
    const VSFormat *rfi = nullptr;

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
    virtual void process_core8() override { process_core<uint8_t>(); }
    virtual void process_core16() override { process_core<uint16_t>(); }
    virtual void process_coreS() override { process_core<float>(); }

public:
    BM3D_Process_Base(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
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

protected:
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
            vsapi->logMessage(mtWarning, "bm3d.Basic/bm3d.Final - warning: "
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

        // The output frame
        _NewFrame(width, height, dfi == fi);
    }

    void Kernel(FLType *dst, const FLType *src, const FLType *ref) const;

    void Kernel(FLType *dstY, FLType *dstU, FLType *dstV,
        const FLType *srcY, const FLType *srcU, const FLType *srcV,
        const FLType *refY, const FLType *refU, const FLType *refV) const;

    PosPairCode BlockMatching(const FLType *ref, PCType j, PCType i) const;

    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &code) const = 0;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "BM3D_Base.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
