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


#ifndef SPECIFICATION_H_
#define SPECIFICATION_H_


#include "Type.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const PCType HD_Width_U = 2048;
const PCType HD_Height_U = 1536;
const PCType SD_Width_U = 1024;
const PCType SD_Height_U = 576;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


enum class ResLevel
{
    SD = 0,
    HD,
    UHD,
    Unknown,
};


enum class ColorMatrix
{
    GBR = 0,
    bt709 = 1,
    Unspecified = 2,
    fcc = 4,
    bt470bg = 5,
    smpte170m = 6,
    smpte240m = 7,
    YCgCo = 8,
    bt2020nc = 9,
    bt2020c = 10,
    OPP = 100, // opponent colorspace
    Minimum,
    Maximum
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Parameter functions
template < typename T >
void ColorMatrix_Parameter(ColorMatrix _ColorMatrix, T &Kr, T &Kg, T &Kb)
{
    switch (_ColorMatrix)
    {
    case ColorMatrix::GBR:
        Kr = T(0);
        Kg = T(1);
        Kb = T(0);
        break;
    case ColorMatrix::bt709:
        Kr = T(0.2126);
        Kg = T(0.7152);
        Kb = T(0.0722);
        break;
    case ColorMatrix::fcc:
        Kr = T(0.30);
        Kg = T(0.59);
        Kb = T(0.11);
        break;
    case ColorMatrix::bt470bg:
        Kr = T(0.299);
        Kg = T(0.587);
        Kb = T(0.114);
        break;
    case ColorMatrix::smpte170m:
        Kr = T(0.299);
        Kg = T(0.587);
        Kb = T(0.114);
        break;
    case ColorMatrix::smpte240m:
        Kr = T(0.212);
        Kg = T(0.701);
        Kb = T(0.087);
        break;
    case ColorMatrix::YCgCo:
        Kr = T(0.25);
        Kg = T(0.50);
        Kb = T(0.25);
        break;
    case ColorMatrix::bt2020nc:
        Kr = T(0.2627);
        Kg = T(0.6780);
        Kb = T(0.0593);
        break;
    case ColorMatrix::bt2020c:
        Kr = T(0.2627);
        Kg = T(0.6780);
        Kb = T(0.0593);
        break;
    case ColorMatrix::OPP:
        Kr = T(1. / 3.);
        Kg = T(1. / 3.);
        Kb = T(1. / 3.);
        break;
    default:
        Kr = T(0.2126);
        Kg = T(0.7152);
        Kb = T(0.0722);
        break;
    }
}


template < typename T >
void ColorMatrix_RGB2YUV_Parameter(ColorMatrix _ColorMatrix, T &Yr, T &Yg, T &Yb, T &Ur, T &Ug, T &Ub, T &Vr, T &Vg, T &Vb)
{
    if (_ColorMatrix == ColorMatrix::GBR)
    {
        // E'Y = 1 * E'G
        Yr = static_cast<T>(0.0L);
        Yg = static_cast<T>(1.0L);
        Yb = static_cast<T>(0.0L);

        // E'Pb = 1 * E'B
        Ur = static_cast<T>(0.0L);
        Ug = static_cast<T>(0.0L);
        Ub = static_cast<T>(1.0L);

        // E'Pr = 1 * E'R
        Vr = static_cast<T>(1.0L);
        Vg = static_cast<T>(0.0L);
        Vb = static_cast<T>(0.0L);
    }
    else if (_ColorMatrix == ColorMatrix::YCgCo)
    {
        // E'Y  =   1 / 4 * E'R + 1 / 2 * E'G + 1 / 4 * E'B
        Yr = static_cast<T>(1.0L / 4.0L);
        Yg = static_cast<T>(1.0L / 2.0L);
        Yb = static_cast<T>(1.0L / 4.0L);

        // E'Pg = - 1 / 4 * E'R + 1 / 2 * E'G - 1 / 4 * E'B
        Ur = static_cast<T>(-1.0L / 4.0L);
        Ug = static_cast<T>(1.0L / 2.0L);
        Ub = static_cast<T>(-1.0L / 4.0L);

        // E'Po = 1 / 2 * E'R                 - 1 / 2 * E'B
        Vr = static_cast<T>(1.0L / 2.0L);
        Vg = static_cast<T>(0.0L);
        Vb = static_cast<T>(-1.0L / 2.0L);
    }
    else if (_ColorMatrix == ColorMatrix::OPP)
    {
        // E'Y  = 1 / 3 * E'R + 1 / 3 * E'G + 1 / 3 * E'B
        Yr = static_cast<T>(1.0L / 3.0L);
        Yg = static_cast<T>(1.0L / 3.0L);
        Yb = static_cast<T>(1.0L / 3.0L);

        // E'Pb = 1 / 2 * E'R               - 1 / 2 * E'B
        Ur = static_cast<T>(1.0L / 2.0L);
        Ug = static_cast<T>(0.0L);
        Ub = static_cast<T>(-1.0L / 2.0L);

        // E'Pr = 1 / 4 * E'R - 1 / 2 * E'G + 1 / 4 * E'B
        Vr = static_cast<T>(1.0L / 4.0L);
        Vg = static_cast<T>(-1.0L / 2.0L);
        Vb = static_cast<T>(1.0L / 4.0L);
    }
    else
    {
        ldbl Kr, Kg, Kb;
        ColorMatrix_Parameter(_ColorMatrix, Kr, Kg, Kb);

        // E'Y = Kr * E'R + ( 1 - Kr - Kg ) * E'G + Kb * E'B
        Yr = static_cast<T>(Kr);
        Yg = static_cast<T>(Kg);
        Yb = static_cast<T>(Kb);

        // E'Pb = 0.5 * ( E'B - E'Y ) / ( 1 - Kb )
        Ur = static_cast<T>(-Yr * 0.5L / (1.0L - Yb));
        Ug = static_cast<T>(-Yg * 0.5L / (1.0L - Yb));
        Ub = static_cast<T>(0.5L);

        // E'Pr = 0.5 * ( E'R - E'Y ) / ( 1 - Kr )
        Vr = static_cast<T>(0.5L);
        Vg = static_cast<T>(-Yg * 0.5L / (1.0L - Yr));
        Vb = static_cast<T>(-Yb * 0.5L / (1.0L - Yr));
    }
}


template < typename T >
void ColorMatrix_YUV2RGB_Parameter(ColorMatrix _ColorMatrix, T &Ry, T &Ru, T &Rv, T &Gy, T &Gu, T &Gv, T &By, T &Bu, T &Bv)
{
    if (_ColorMatrix == ColorMatrix::GBR)
    {
        // E'R = 1 * E'Pr
        Ry = static_cast<T>(0.0L);
        Ru = static_cast<T>(0.0L);
        Rv = static_cast<T>(1.0L);

        // E'G = 1 * E'Y
        Gy = static_cast<T>(1.0L);
        Gu = static_cast<T>(0.0L);
        Gv = static_cast<T>(0.0L);

        // E'B = 1 * E'Pb
        By = static_cast<T>(0.0L);
        Bu = static_cast<T>(1.0L);
        Bv = static_cast<T>(0.0L);
    }
    else if (_ColorMatrix == ColorMatrix::YCgCo)
    {
        // E'R = E'Y - E'Pg + E'Po
        Ry = static_cast<T>(1.0L);
        Ru = static_cast<T>(-1.0L);
        Rv = static_cast<T>(1.0L);

        // E'G = E'Y + E'Pg
        Gy = static_cast<T>(1.0L);
        Gu = static_cast<T>(1.0L);
        Gv = static_cast<T>(0.0L);

        // E'B = E'Y - E'Pg - E'Po
        By = static_cast<T>(1.0L);
        Bu = static_cast<T>(-1.0L);
        Bv = static_cast<T>(-1.0L);
    }
    else if (_ColorMatrix == ColorMatrix::OPP)
    {
        // E'R = E'Y + E'Pb + 2 / 3 * E'Pr
        Ry = static_cast<T>(1.0L);
        Ru = static_cast<T>(1.0L);
        Rv = static_cast<T>(2.0L / 3.0L);

        // E'G = E'Y        - 4 / 3 * E'Pr
        Gy = static_cast<T>(1.0L);
        Gu = static_cast<T>(0.0L);
        Gv = static_cast<T>(-4.0L / 3.0L);

        // E'B = E'Y - E'Pb + 2 / 3 * E'Pr
        By = static_cast<T>(1.0L);
        Bu = static_cast<T>(-1.0L);
        Bv = static_cast<T>(2.0L / 3.0L);
    }
    else
    {
        ldbl Kr, Kg, Kb;
        ColorMatrix_Parameter(_ColorMatrix, Kr, Kg, Kb);

        // E'R = E'Y + 2 * ( 1 - Kr ) * E'Pr
        Ry = static_cast<T>(1.0L);
        Ru = static_cast<T>(0.0L);
        Rv = static_cast<T>(2.0L * (1.0L - Kr));

        // E'G = E'Y - 2 * Kb * ( 1 - Kb ) / ( 1 - Kr - Kb ) * E'Pb - 2 * Kr * ( 1 - Kr ) / ( 1 - Kr - Kb ) * E'Pr
        Gy = static_cast<T>(1.0L);
        Gu = static_cast<T>(2.0L * Kb * (1.0L - Kb) / Kg);
        Gv = static_cast<T>(2.0L * Kr * (1.0L - Kr) / Kg);

        // E'B = E'Y + 2 * ( 1 - Kb ) * E'Pb
        By = static_cast<T>(1.0L);
        Bu = static_cast<T>(2.0L * (1.0L - Kb));
        Bv = static_cast<T>(0.0L);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Default functions
inline ResLevel ResLevel_Default(int Width, int Height)
{
    if (Width > HD_Width_U || Height > HD_Height_U) return ResLevel::UHD;
    if (Width > SD_Width_U || Height > SD_Height_U) return ResLevel::HD;
    return ResLevel::SD;
}


inline ColorMatrix ColorMatrix_Default(int Width, int Height)
{
    ResLevel _ResLevel = ResLevel_Default(Width, Height);

    if (_ResLevel == ResLevel::UHD) return ColorMatrix::bt2020nc;
    if (_ResLevel == ResLevel::HD) return ColorMatrix::bt709;
    if (_ResLevel == ResLevel::SD) return ColorMatrix::smpte170m;
    return ColorMatrix::bt709;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
