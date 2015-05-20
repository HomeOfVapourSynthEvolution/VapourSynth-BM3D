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


#include "BM3D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of struct BM3D_Para


BM3D_Para::BM3D_Para(bool _wiener, std::string _profile)
    : wiener(_wiener), profile(_profile), sigma({ 10.0, 10.0, 10.0 })
{
    BlockSize = 8;
    BlockStep = 3;
    BMrange = 16;
    BMstep = 1;

    if (!wiener)
    {
        GroupSize = 16;
        lambda = 2.5;
    }
    else
    {
        GroupSize = 32;
    }

    if (profile == "lc")
    {
        BMrange = 9;

        if (!wiener)
        {
            BlockStep = 6;
        }
        else
        {
            BlockStep = 5;
            GroupSize = 16;
        }
    }
    else if (profile == "vn")
    {
        if (!wiener)
        {
            BlockStep = 4;
            GroupSize = 32;
            lambda = 2.6;
        }
        else
        {
            BlockSize = 11;
            BlockStep = 6;
        }
    }
    else if (profile == "high")
    {
        BlockStep = 2;
    }

    thMSE_Default();
}


void BM3D_Para::thMSE_Default()
{
    if (!wiener)
    {
        thMSE = 400 + sigma[0] * 80;

        if (profile == "vn")
        {
            thMSE = 1000 + sigma[0] * 150;
        }
    }
    else
    {
        thMSE = 200 + sigma[0] * 10;

        if (profile == "vn")
        {
            thMSE = 400 + sigma[0] * 40;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of struct BM3D_FilterData


BM3D_FilterData::BM3D_FilterData(bool wiener, double sigma, PCType GroupSize, PCType BlockSize, double lambda)
    : fp(GroupSize), bp(GroupSize), finalAMP(GroupSize), thrTable(wiener ? 0 : GroupSize),
    wienerSigmaSqr(wiener ? GroupSize : 0)
{
    const unsigned int flags = FFTW_PATIENT;
    const fftw::r2r_kind fkind = FFTW_REDFT01;
    const fftw::r2r_kind bkind = FFTW_REDFT10;

    FLType *temp = nullptr;

    for (PCType i = 1; i <= GroupSize; ++i)
    {
        AlignedMalloc(temp, i * BlockSize * BlockSize);
        fp[i - 1].r2r_3d(i, BlockSize, BlockSize, temp, temp, fkind, fkind, fkind, flags);
        bp[i - 1].r2r_3d(i, BlockSize, BlockSize, temp, temp, bkind, bkind, bkind, flags);
        AlignedFree(temp);

        finalAMP[i - 1] = 2 * i * 2 * BlockSize * 2 * BlockSize;
        double forwardAMP = sqrt(finalAMP[i - 1]);

        if (wiener)
        {
            wienerSigmaSqr[i - 1] = static_cast<FLType>(sigma * forwardAMP * sigma * forwardAMP);
        }
        else
        {
            double thrBase = sigma * lambda * forwardAMP;
            std::vector<double> thr(4);
            thr[0] = thrBase;
            thr[1] = thrBase;
            thr[2] = thrBase / 2.;
            thr[3] = 0;

            thrTable[i - 1] = std::vector<FLType>(i * BlockSize * BlockSize);
            auto thr_d = thrTable[i - 1].data();

            for (PCType z = 0; z < i; ++z)
            {
                for (PCType y = 0; y < BlockSize; ++y)
                {
                    for (PCType x = 0; x < BlockSize; ++x, ++thr_d)
                    {
                        int flag = 0;
                        double scale = 1;

                        if (x == 0)
                        {
                            ++flag;
                        }
                        else if (x >= BlockSize / 2)
                        {
                            scale *= 1.07;
                        }
                        else if (x >= BlockSize / 4)
                        {
                            scale *= 1.01;
                        }

                        if (y == 0)
                        {
                            ++flag;
                        }
                        else if (y >= BlockSize / 2)
                        {
                            scale *= 1.07;
                        }
                        else if (y >= BlockSize / 4)
                        {
                            scale *= 1.01;
                        }

                        if (z == 0)
                        {
                            ++flag;
                        }
                        else if (z >= i / 2)
                        {
                            scale *= 1.07;
                        }
                        else if (z >= i / 4)
                        {
                            scale *= 1.01;
                        }

                        *thr_d = static_cast<FLType>(thr[flag] * scale);
                    }
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
