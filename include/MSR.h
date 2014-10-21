/*
* Retinex filter - VapourSynth plugin
* Copyright (C) 2014  mawen1250
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


#ifndef MSR_H_
#define MSR_H_


#include "Helper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class MSRData
{
public:
    const VSAPI *vsapi = nullptr;
    VSNodeRef *node = nullptr;
    const VSVideoInfo *vi = nullptr;

    std::vector<double> sigma;
    double lower_thr = 0;
    double upper_thr = 0;
    bool fulls = true;
    bool fulld = fulls;

    int process[3];

    int HistBins = 4096;

public:
    MSRData(const VSAPI *_vsapi = nullptr)
        : vsapi(_vsapi)
    {
        for (int i = 0; i < 3; i++)
            process[i] = 1;
    }

    virtual ~MSRData()
    {
        if (node) vsapi->freeNode(node);
    }

    void fulls_select()
    {
        if (vi->format->colorFamily == cmGray || vi->format->colorFamily == cmYUV)
            fulls = false;
        else if (vi->format->colorFamily == cmRGB || vi->format->colorFamily == cmYCoCg)
            fulls = true;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int Retinex_MSR(FLType *odata, const FLType *idata, const MSRData &d, int height, int width, int stride);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif