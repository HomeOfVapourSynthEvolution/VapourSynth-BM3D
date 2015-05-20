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


#ifndef BM3D_H_
#define BM3D_H_


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

    explicit BM3D_Para(bool _wiener, std::string _profile = "lc");

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
    std::vector<std::vector<FLType>> thrTable;
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
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
