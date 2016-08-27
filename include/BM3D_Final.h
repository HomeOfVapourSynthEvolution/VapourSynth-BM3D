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


#ifndef BM3D_FINAL_H_
#define BM3D_FINAL_H_


#include "BM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Final_Data
    : public BM3D_Data_Base
{
public:
    typedef BM3D_Final_Data _Myt;
    typedef BM3D_Data_Base _Mybase;

public:
    BM3D_Final_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "Final", std::string _NameSpace = "bm3d")
        : _Mybase(true, _vsapi, _FunctionName, _NameSpace)
    {}

    BM3D_Final_Data(const _Myt &right) = delete;
    BM3D_Final_Data(_Myt &&right) = delete;
    _Myt &operator=(const _Myt &right) = delete;
    _Myt &operator=(_Myt &&right) = delete;

    virtual ~BM3D_Final_Data() override {}

    virtual int arguments_process(const VSMap *in, VSMap *out) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Final_Process
    : public BM3D_Process_Base
{
public:
    typedef BM3D_Final_Process _Myt;
    typedef BM3D_Process_Base _Mybase;
    typedef BM3D_Final_Data _Mydata;

private:
    const _Mydata &d;

public:
    BM3D_Final_Process(_Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {}

    virtual ~BM3D_Final_Process() override {}

protected:
    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &code) const override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
