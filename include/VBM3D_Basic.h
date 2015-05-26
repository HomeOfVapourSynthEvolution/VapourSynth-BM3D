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


#ifndef VBM3D_BASIC_H_
#define VBM3D_BASIC_H_


#include "VBM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VBM3D_Basic_Data
    : public VBM3D_Data_Base
{
public:
    typedef VBM3D_Basic_Data _Myt;
    typedef VBM3D_Data_Base _Mybase;

public:
    VBM3D_Basic_Data(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "VBasic", std::string _NameSpace = "bm3d")
        : _Mybase(false, _vsapi, _FunctionName, _NameSpace)
    {}

    VBM3D_Basic_Data(const _Myt &right) = delete;

    VBM3D_Basic_Data(_Myt &&right)
        : _Mybase(std::move(right))
    {}

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        _Mybase::operator=(std::move(right));

        return *this;
    }

    virtual ~VBM3D_Basic_Data() override {}

    virtual int arguments_process(const VSMap *in, VSMap *out) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VBM3D_Basic_Process
    : public VBM3D_Process_Base
{
public:
    typedef VBM3D_Basic_Process _Myt;
    typedef VBM3D_Process_Base _Mybase;
    typedef VBM3D_Basic_Data _Mydata;

private:
    const _Mydata &d;

public:
    VBM3D_Basic_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {}

    virtual ~VBM3D_Basic_Process() override {}

protected:
    virtual void CollaborativeFilter(int plane,
        const std::vector<FLType *> &ResNum, const std::vector<FLType *> &ResDen,
        const std::vector<const FLType *> &src, const std::vector<const FLType *> &ref,
        const Pos3PairCode &code) const override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
