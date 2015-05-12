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


#ifndef BM3D_BASIC_H_
#define BM3D_BASIC_H_


#include "BM3D_Base.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const struct BM3D_Basic_Para
    : public BM3D_Para_Base
{
    typedef BM3D_Basic_Para _Myt;
    typedef BM3D_Para_Base _Mybase;

    _Myt(std::string _profile = "lc")
        : _Mybase(_profile)
    {
        BlockSize = 8;
        BlockStep = 3;
        GroupSize = 16;
        lambda = 2.5;

        if (profile == "lc")
        {
            BlockStep = 6;
        }
        else if (profile == "vn")
        {
            BlockStep = 4;
            GroupSize = 32;
            lambda = 2.6;
        }
        else if (profile == "high")
        {
            BlockStep = 2;
        }

        thMSE_Default();
    }

    virtual void thMSE_Default() override
    {
        thMSE = 400 + sigma[0] * 80;

        if (profile == "vn")
        {
            thMSE = 1000 + sigma[0] * 150;
        }
    }
} BM3D_Basic_Default;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Basic_Data
    : public BM3D_Data_Base
{
public:
    typedef BM3D_Basic_Data _Myt;
    typedef BM3D_Data_Base _Mybase;
    typedef BM3D_Basic_Para _Mypara;

public:
    _Myt(const VSAPI *_vsapi = nullptr, std::string _FunctionName = "Basic", std::string _NameSpace = "bm3d",
        const _Mypara &_para = BM3D_Basic_Default)
        : _Mybase(_vsapi, _FunctionName, _NameSpace, _para, false)
    {}

    _Myt(const _Myt &right) = delete;

    _Myt(_Myt &&right)
        : _Mybase(std::move(right))
    {}

    _Myt &operator=(const _Myt &right) = delete;

    _Myt &operator=(_Myt &&right)
    {
        _Mybase::operator=(std::move(right));

        return *this;
    }

    virtual ~BM3D_Basic_Data() override {}

    virtual int arguments_process(const VSMap *in, VSMap *out) override;

protected:
    virtual void get_default_para(std::string _profile = "lc") override
    {
        para_default = _Mypara(_profile);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class BM3D_Basic_Process
    : public BM3D_Process_Base
{
public:
    typedef BM3D_Basic_Process _Myt;
    typedef BM3D_Process_Base _Mybase;
    typedef BM3D_Basic_Data _Mydata;

private:
    const _Mydata &d;

public:
    _Myt(const _Mydata &_d, int n, VSFrameContext *frameCtx, VSCore *core, const VSAPI *_vsapi)
        : _Mybase(_d, n, frameCtx, core, _vsapi), d(_d)
    {}

    virtual ~BM3D_Basic_Process() override {}

protected:
    virtual void CollaborativeFilter(int plane,
        FLType *ResNum, FLType *ResDen,
        const FLType *src, const FLType *ref,
        const PosPairCode &posPairCode) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
