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


#include "BM3D_Basic.h"
#include "BM3D_Final.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const struct BM3D_Para
{
    BM3D_Basic_Para basic;
    BM3D_Final_Para final;

    BM3D_Para(std::string _profile = "lc")
        : basic(_profile), final(_profile)
    {}

    void thMSE_Default()
    {
        basic.thMSE_Default();
        final.thMSE_Default();
    }
} BM3D_Default;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
