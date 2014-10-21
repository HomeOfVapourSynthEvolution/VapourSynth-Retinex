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


#ifndef MSRCP_H_
#define MSRCP_H_


#include "Helper.h"
#include "MSR.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class MSRCPData
    : public MSRData
{
public:
    double chroma_protect = 1.2;

public:
    MSRCPData(const VSAPI *_vsapi = nullptr)
        : MSRData(_vsapi) {}

    ~MSRCPData() {}
};


void VS_CC MSRCPCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename T >
void Retinex_MSRCP(VSFrameRef * dst, const VSFrameRef * src, const VSAPI * vsapi, const MSRCPData &d)
{
    int i, j, upper;
    const T *Ysrcp;
    T *Ydstp;
    int stride, width, height, pcount;

    const VSFormat *fi = vsapi->getFrameFormat(src);

    int bps = fi->bitsPerSample;
    T sFloor = 0;
    //T sFloorC = 0;
    int sNeutral = 128 << (bps - 8);
    T sCeil = (1 << bps) - 1;
    //T sCeilC = (1 << bps) - 1;
    T sRange = d.fulls ? (1 << bps) - 1 : 219 << (bps - 8);
    T sRangeC = d.fulls ? (1 << bps) - 1 : 224 << (bps - 8);
    T dFloor = d.fulld ? 0 : 16 << (bps - 8);
    //T dFloorC = d.fulld ? 0 : 16 << (bps - 8);
    int dNeutral = 128 << (bps - 8);
    T dCeil = d.fulld ? (1 << bps) - 1 : 235 << (bps - 8);
    //T dCeilC = d.fulld ? (1 << bps) - 1 : 240 << (bps - 8);
    T dRange = d.fulld ? (1 << bps) - 1 : 219 << (bps - 8);
    T dRangeC = d.fulld ? (1 << bps) - 1 : 224 << (bps - 8);
    FLType sFloorFL = static_cast<FLType>(sFloor);
    //FLType sFloorCFL = static_cast<FLType>(sFloorC);
    //FLType sNeutralFL = static_cast<FLType>(sNeutral);
    //FLType sCeilFL = static_cast<FLType>(sCeil);
    //FLType sCeilCFL = static_cast<FLType>(sCeilC);
    FLType sRangeFL = static_cast<FLType>(sRange);
    FLType sRangeCFL = static_cast<FLType>(sRangeC);
    FLType sRangeC2FL = static_cast<FLType>(sRangeC) / 2.;
    FLType dFloorFL = static_cast<FLType>(dFloor);
    //FLType dFloorCFL = static_cast<FLType>(dFloorC);
    FLType dNeutralFL = static_cast<FLType>(dNeutral);
    //FLType dCeilFL = static_cast<FLType>(dCeil);
    //FLType dCeilCFL = static_cast<FLType>(dCeilC);
    FLType dRangeFL = static_cast<FLType>(dRange);
    FLType dRangeCFL = static_cast<FLType>(dRangeC);

    stride = vsapi->getStride(src, 0) / sizeof(T);
    width = vsapi->getFrameWidth(src, 0);
    height = vsapi->getFrameHeight(src, 0);
    pcount = stride * height;

    FLType gain, offset, scale;

    FLType *idata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);
    FLType *odata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);

    if (fi->colorFamily == cmGray)
    {
        Ysrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 0));
        Ydstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 0));

        if (d.fulls)
        {
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = Ysrcp[i] * gain;
            }
        }
        else
        {
            T min, max;

            min = sCeil;
            max = sFloor;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    min = Min(min, Ysrcp[i]);
                    max = Max(max, Ysrcp[i]);
                }
            }
            if (max > min)
            {
                sFloor = min;
                sCeil = max;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
            }

            gain = 1 / static_cast<FLType>(sCeil - sFloor);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Ysrcp[i] - sFloor) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        offset = dFloorFL + FLType(0.5);
        for (j = 0; j < height; j++)
        {
            i = stride * j;
            for (upper = i + width; i < upper; i++)
                Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + offset);
        }
    }
    else if (fi->colorFamily == cmRGB)
    {
        const T *Rsrcp, *Gsrcp, *Bsrcp;
        T *Rdstp, *Gdstp, *Bdstp;

        Rsrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 0));
        Rdstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 0));
        Gsrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 1));
        Gdstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 1));
        Bsrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 2));
        Bdstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 2));

        if (d.fulls)
        {
            gain = 1 / (sRangeFL * 3);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Rsrcp[i] + Gsrcp[i] + Bsrcp[i]) * gain;
            }
        }
        else
        {
            T min, max;

            min = sCeil;
            max = sFloor;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    min = Min(min, Min(Rsrcp[i], Min(Gsrcp[i], Bsrcp[i])));
                    max = Max(max, Max(Rsrcp[i], Max(Gsrcp[i], Bsrcp[i])));
                }
            }
            if (max > min)
            {
                sFloor = min;
                sCeil = max;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
            }

            offset = sFloorFL * -3;
            gain = 1 / (static_cast<FLType>(sCeil - sFloor) * 3);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Rsrcp[i] + Gsrcp[i] + Bsrcp[i] + offset) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        if (sFloor == 0 && dFloorFL == 0 && sRangeFL == dRangeFL)
        {
            offset = FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = idata[i] <= 0 ? 1 : odata[i] / idata[i];
                    gain = Min(sRangeFL / Max(Rsrcp[i], Max(Gsrcp[i], Bsrcp[i])), gain);
                    Rdstp[i] = static_cast<T>(Rsrcp[i] * gain + offset);
                    Gdstp[i] = static_cast<T>(Gsrcp[i] * gain + offset);
                    Bdstp[i] = static_cast<T>(Bsrcp[i] * gain + offset);
                }
            }
        }
        else
        {
            scale = dRangeFL / sRangeFL;
            offset = dFloorFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = idata[i] <= 0 ? 1 : odata[i] / idata[i];
                    gain = Min(sRangeFL / Max(Rsrcp[i], Max(Gsrcp[i], Bsrcp[i])), gain) * scale;
                    Rdstp[i] = static_cast<T>((Rsrcp[i] - sFloor) * gain + offset);
                    Gdstp[i] = static_cast<T>((Gsrcp[i] - sFloor) * gain + offset);
                    Bdstp[i] = static_cast<T>((Bsrcp[i] - sFloor) * gain + offset);
                }
            }
        }
    }
    else
    {
        const T *Usrcp, *Vsrcp;
        T *Udstp, *Vdstp;

        Ysrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 0));
        Ydstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 0));
        Usrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 1));
        Udstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 1));
        Vsrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 2));
        Vdstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 2));

        if (d.fulls)
        {
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = Ysrcp[i] * gain;
            }
        }
        else
        {
            T min, max;

            min = sCeil;
            max = sFloor;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    min = Min(min, Ysrcp[i]);
                    max = Max(max, Ysrcp[i]);
                }
            }
            if (max > min)
            {
                sFloor = min;
                sCeil = max;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
            }

            gain = 1 / static_cast<FLType>(sCeil - sFloor);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Ysrcp[i] - sFloor) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        FLType chroma_protect_mul1 = static_cast<FLType>(d.chroma_protect - 1);
        FLType chroma_protect_mul2 = static_cast<FLType>(1 / log(d.chroma_protect));

        int Uval, Vval;
        scale = dRangeCFL / sRangeCFL;
        if (d.fulld)
            offset = dNeutralFL + FLType(0.499999);
        else
            offset = dNeutralFL + FLType(0.5);

        for (j = 0; j < height; j++)
        {
            i = stride * j;
            for (upper = i + width; i < upper; i++)
            {
                if (d.chroma_protect > 1)
                    gain = idata[i] <= 0 ? 1 : log(odata[i] / idata[i] * chroma_protect_mul1 + 1) * chroma_protect_mul2;
                else
                    gain = idata[i] <= 0 ? 1 : odata[i] / idata[i];
                Uval = Usrcp[i] - sNeutral;
                Vval = Vsrcp[i] - sNeutral;
                if (dRangeCFL == sRangeCFL)
                    gain = Min(sRangeC2FL / Max(Abs(Uval), Abs(Vval)), gain);
                else
                    gain = Min(sRangeC2FL / Max(Abs(Uval), Abs(Vval)), gain) * scale;
                Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + dFloorFL + FLType(0.5));
                Udstp[i] = static_cast<T>(Uval * gain + offset);
                Vdstp[i] = static_cast<T>(Vval * gain + offset);
            }
        }
    }

    vs_aligned_free(idata);
    vs_aligned_free(odata);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif