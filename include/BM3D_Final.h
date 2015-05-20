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

    BM3D_Final_Data(_Myt &&right)
        : _Mybase(std::move(right))
    {}

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        _Mybase::operator=(std::move(right));

        return *this;
    }

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
    BM3D_Final_Process(const _Mydata &_d, int _n, VSFrameContext *_frameCtx, VSCore *_core, const VSAPI *_vsapi)
        : _Mybase(_d, _n, _frameCtx, _core, _vsapi), d(_d)
    {}

    virtual ~BM3D_Final_Process() override {}

protected:
    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &code) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
