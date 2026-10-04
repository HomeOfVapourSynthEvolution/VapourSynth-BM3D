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

#include <limits>
#include "BM3D.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions of struct BM3D_Para


BM3D_Para::BM3D_Para(bool _wiener, std::string _profile)
    : wiener(_wiener), profile(_profile), sigma({ 10.0, 10.0, 10.0 })
{
    BlockSize = 8;
    BMrange = 16;
    BMstep = 1;

    if (!wiener)
    {
        BlockStep = 4;
        GroupSize = 16;
        lambda = 2.7;
    }
    else
    {
        BlockStep = 3;
        GroupSize = 32;
    }

    if (profile == "fast")
    {
        BMrange = 9;
        GroupSize = 8;

        if (!wiener)
        {
            BlockStep = 8;
        }
        else
        {
            BlockStep = 7;
        }
    }
    else if (profile == "lc")
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
    else if (profile == "high")
    {
        if (!wiener)
        {
            BlockStep = 3;
        }
        else
        {
            BlockStep = 2;
        }
    }
    else if (profile == "vn")
    {
        if (!wiener)
        {
            BlockStep = 4;
            GroupSize = 32;
            lambda = 2.8;
        }
        else
        {
            BlockSize = 11;
            BlockStep = 6;
        }
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
    const fftw::r2r_kind fkind = FFTW_REDFT10;
    const fftw::r2r_kind bkind = FFTW_REDFT01;

    FLType *temp = nullptr;

    for (PCType i = 1; i <= GroupSize; ++i)
    {
        AlignedMalloc(temp, i * BlockSize * BlockSize);
        fp[i - 1].r2r_3d(i, BlockSize, BlockSize, temp, temp, fkind, fkind, fkind, flags);
        bp[i - 1].r2r_3d(i, BlockSize, BlockSize, temp, temp, bkind, bkind, bkind, flags);
        AlignedFree(temp);

        finalAMP[i - 1] = 2 * i * 2 * BlockSize * 2 * BlockSize;
        double forwardAMP = sqrt(finalAMP[i - 1]);

        // Unnormalized DCT-II doubles the noise variance of zero-frequency coefficients along each axis, indexed by flag below
        std::vector<double> coef(4);

        if (wiener)
        {
            // Floor keeps the Wiener coefficient finite when sigma is 0 and the reference coefficient is 0
            const double sigmaSqrBase = std::max(sigma * forwardAMP * sigma * forwardAMP, double(std::numeric_limits<FLType>::min()));

            for (int flag = 0; flag < 4; ++flag)
            {
                coef[flag] = sigmaSqrBase * (1 << flag);
            }
        }
        else
        {
            double thrBase = sigma * lambda * forwardAMP;

            coef[0] = thrBase;
            coef[1] = thrBase * sqrt(double(2));
            coef[2] = thrBase * double(2);
            coef[3] = thrBase * sqrt(double(8));
        }

        FLType *coefp = nullptr;
        AlignedMalloc(coefp, i * BlockSize * BlockSize);
        (wiener ? wienerSigmaSqr : thrTable)[i - 1].reset(coefp, [](FLType *memory)
        {
            AlignedFree(memory);
        });

        for (PCType z = 0; z < i; ++z)
        {
            for (PCType y = 0; y < BlockSize; ++y)
            {
                for (PCType x = 0; x < BlockSize; ++x, ++coefp)
                {
                    int flag = 0;

                    if (x == 0)
                    {
                        ++flag;
                    }
                    if (y == 0)
                    {
                        ++flag;
                    }
                    if (z == 0)
                    {
                        ++flag;
                    }

                    *coefp = static_cast<FLType>(coef[flag]);
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
