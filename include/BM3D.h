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


#ifndef BM3D_H_
#define BM3D_H_


#include <memory>
#include "fftw3_helper.hpp"
#include "Conversion.hpp"
#include "Block.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct BM3D_Para
{
    bool wiener;

    std::string profile;
    std::vector<double> sigma;
    PCType BlockSize;
    PCType BlockStep;
    PCType GroupSize;
    PCType BMrange;
    PCType BMstep;
    double thMSE;
    double lambda;

    explicit BM3D_Para(bool _wiener, std::string _profile = "fast");

    void thMSE_Default();
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct BM3D_FilterData
{
    typedef BM3D_FilterData _Myt;

    typedef fftwh<FLType> fftw;

    std::vector<fftw::plan> fp;
    std::vector<fftw::plan> bp;
    std::vector<double> finalAMP;
    std::vector<std::shared_ptr<const FLType>> thrTable;
    std::vector<FLType> wienerSigmaSqr;

    BM3D_FilterData() {}

    BM3D_FilterData(bool wiener, double sigma, PCType GroupSize, PCType BlockSize, double lambda);

    BM3D_FilterData(const _Myt &right) = delete;

    BM3D_FilterData(_Myt &&right)
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
    
    static std::mutex s_fftw_planner_mutex;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
