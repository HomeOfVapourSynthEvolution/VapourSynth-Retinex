#ifndef MSRCP_H_
#define MSRCP_H_


#include "Helper.h"
#include "Specification.h"
#include "MSR.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class MSRCPData
    : public MSRData
{
public:
    ColorMatrix ColorMatrix_ = ColorMatrix::Unspecified;

public:
    MSRCPData(const VSAPI *_vsapi = nullptr)
        : MSRData(_vsapi) {}

    ~MSRCPData() {}

    void ColorMatrix_select()
    {
        if (ColorMatrix_ == ColorMatrix::Unspecified)
            ColorMatrix_ = ColorMatrix_Default(vi->width, vi->height);
    }
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
    T sRange = sCeil - sFloor;
    T sRangeC = d.fulls ? (1 << bps) - 1 : 224 << (bps - 8);
    T dFloor = d.fulld ? 0 : 16 << (bps - 8);
    //T dFloorC = d.fulld ? 0 : 16 << (bps - 8);
    int dNeutral = 128 << (bps - 8);
    T dCeil = d.fulld ? (1 << bps) - 1 : 235 << (bps - 8);
    //T dCeilC = d.fulld ? (1 << bps) - 1 : 240 << (bps - 8);
    T dRange = dCeil - dFloor;
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
                sRange = sCeil - sFloor;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
                sRangeFL = static_cast<FLType>(sRange);
            }

            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Ysrcp[i] - sFloor) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        if (d.fulld)
        {
            offset = FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + offset);
            }
        }
        else
        {
            offset = dFloorFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + offset);
            }
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

        FLType Kr, Kg, Kb;
        ColorMatrix_Parameter(d.ColorMatrix_, Kr, Kg, Kb);

        if (d.fulls)
        {
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Kr*Rsrcp[i] + Kg*Gsrcp[i] + Kb*Bsrcp[i]) * gain;
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
                sRange = sCeil - sFloor;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
                sRangeFL = static_cast<FLType>(sRange);
            }

            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Kr*Rsrcp[i] + Kg*Gsrcp[i] + Kb*Bsrcp[i] - sFloorFL) * gain;
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
                    gain = Min(sRangeFL / Max(Rsrcp[i], Max(Gsrcp[i], Bsrcp[i])), idata[i] <= 0 ? 1 : odata[i] / idata[i]);
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
                    gain = Min(sRangeFL / Max(Rsrcp[i], Max(Gsrcp[i], Bsrcp[i])), idata[i] <= 0 ? 1 : odata[i] / idata[i]) * scale;
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
                sRange = sCeil - sFloor;
                sFloorFL = static_cast<FLType>(sFloor);
                //sCeilFL = static_cast<FLType>(sCeil);
                sRangeFL = static_cast<FLType>(sRange);
            }

            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Ysrcp[i] - sFloor) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        if (dRangeCFL == sRangeCFL)
        {
            offset = dNeutralFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = Min(sRangeC2FL / Max(Abs(Usrcp[i] - sNeutral), Abs(Vsrcp[i] - sNeutral)), idata[i] <= 0 ? 1 : odata[i] / idata[i]);
                    Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + dFloorFL + FLType(0.5));
                    Udstp[i] = static_cast<T>((Usrcp[i] - sNeutral) * gain + offset);
                    Vdstp[i] = static_cast<T>((Vsrcp[i] - sNeutral) * gain + offset);
                }
            }
        }
        else
        {
            scale = dRangeCFL / sRangeCFL;
            offset = dNeutralFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = Min(sRangeC2FL / Max(Abs(Usrcp[i] - sNeutral), Abs(Vsrcp[i] - sNeutral)), idata[i] <= 0 ? 1 : odata[i] / idata[i]) * scale;
                    Ydstp[i] = static_cast<T>(odata[i] * dRangeFL + dFloorFL + FLType(0.5));
                    Udstp[i] = static_cast<T>((Usrcp[i] - sNeutral) * gain + offset);
                    Vdstp[i] = static_cast<T>((Vsrcp[i] - sNeutral) * gain + offset);
                }
            }
        }
    }

    vs_aligned_free(idata);
    vs_aligned_free(odata);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif