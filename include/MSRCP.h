#ifndef MSRCP_H_
#define MSRCP_H_


#include <vapoursynth\VapourSynth.h>
#include <vapoursynth\VSHelper.h>
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
    T sFloor = d.fulls ? 0 : 16 << (bps - 8);
    int sNeutral = 128 << (bps - 8);
    T sCeil = d.fulls ? (1 << bps) - 1 : 235 << (bps - 8);
    T sCeilC = d.fulls ? (1 << bps) - 1 : 240 << (bps - 8);
    T sRange = sCeil - sFloor;
    T dFloor = d.fulld ? 0 : 16 << (bps - 8);
    int dNeutral = 128 << (bps - 8);
    T dCeil = d.fulld ? (1 << bps) - 1 : 235 << (bps - 8);
    T dCeilC = d.fulls ? (1 << bps) - 1 : 240 << (bps - 8);
    T dRange = dCeil - dFloor;
    FLType sFloorFL = static_cast<FLType>(sFloor);
    FLType sNeutralFL = static_cast<FLType>(sNeutral);
    FLType sCeilFL = static_cast<FLType>(sCeil);
    FLType sCeilCFL = static_cast<FLType>(sCeilC);
    FLType sRangeFL = static_cast<FLType>(sRange);
    FLType dFloorFL = static_cast<FLType>(dFloor);
    FLType dNeutralFL = static_cast<FLType>(dNeutral);
    FLType dCeilFL = static_cast<FLType>(dCeil);
    FLType dCeilCFL = static_cast<FLType>(dCeilC);
    FLType dRangeFL = static_cast<FLType>(dRange);

    stride = vsapi->getStride(src, 0) / sizeof(T);
    width = vsapi->getFrameWidth(src, 0);
    height = vsapi->getFrameHeight(src, 0);
    pcount = stride * height;

    FLType gain, offset, scale;

    if (fi->colorFamily == cmGray)
    {
        Ysrcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, 0));
        Ydstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, 0));

        FLType *idata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);
        FLType *odata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);

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
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Clip(Ysrcp[i], sFloor, sCeil) - sFloor) * gain;
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
                    Ydstp[i] = static_cast<T>(Clip(odata[i] * dRangeFL + offset, dFloorFL, dCeilFL));
            }
        }
        else
        {
            offset = dFloorFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    Ydstp[i] = static_cast<T>(Clip(odata[i] * dRangeFL + offset, dFloorFL, dCeilFL));
            }
        }

        vs_aligned_free(idata);
        vs_aligned_free(odata);
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

        FLType *idata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);
        FLType *odata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);

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
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Kr*Clip(Rsrcp[i], sFloor, sCeil) + Kg*Clip(Gsrcp[i], sFloor, sCeil) + Kb*Clip(Bsrcp[i], sFloor, sCeil) - sFloorFL) * gain;
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
                    Rdstp[i] = static_cast<T>(Clip(Rsrcp[i] * gain + offset, dFloorFL, dCeilFL));
                    Gdstp[i] = static_cast<T>(Clip(Gsrcp[i] * gain + offset, dFloorFL, dCeilFL));
                    Bdstp[i] = static_cast<T>(Clip(Bsrcp[i] * gain + offset, dFloorFL, dCeilFL));
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
                    gain = idata[i] <= 0 ? scale : odata[i] / idata[i] * scale;
                    Rdstp[i] = static_cast<T>(Clip((Rsrcp[i] - sFloor) * gain + offset, dFloorFL, dCeilFL));
                    Gdstp[i] = static_cast<T>(Clip((Gsrcp[i] - sFloor) * gain + offset, dFloorFL, dCeilFL));
                    Bdstp[i] = static_cast<T>(Clip((Bsrcp[i] - sFloor) * gain + offset, dFloorFL, dCeilFL));
                }
            }
        }

        vs_aligned_free(idata);
        vs_aligned_free(odata);
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

        FLType *idata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);
        FLType *odata = vs_aligned_malloc<FLType>(sizeof(FLType)*pcount, Alignment);

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
            gain = 1 / sRangeFL;
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                    idata[i] = (Clip(Ysrcp[i], sFloor, sCeil) - sFloor) * gain;
            }
        }

        Retinex_MSR(odata, idata, d, height, width, stride);

        if (dCeilCFL - dFloorFL == sCeilCFL - sFloorFL)
        {
            offset = dNeutralFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = idata[i] <= 0 ? 1 : odata[i] / idata[i];
                    Ydstp[i] = static_cast<T>(Clip(odata[i] * dRangeFL + dFloorFL + FLType(0.5), dFloorFL, dCeilFL));
                    Udstp[i] = static_cast<T>(Clip((Usrcp[i] - sNeutral) * gain + offset, dFloorFL, dCeilCFL));
                    Vdstp[i] = static_cast<T>(Clip((Vsrcp[i] - sNeutral) * gain + offset, dFloorFL, dCeilCFL));
                }
            }
        }
        else
        {
            scale = (dCeilCFL - dFloorFL) / (sCeilCFL - sFloorFL);
            offset = dNeutralFL + FLType(0.5);
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    gain = idata[i] <= 0 ? scale : odata[i] / idata[i] * scale;
                    Ydstp[i] = static_cast<T>(Clip(odata[i] * dRangeFL + dFloorFL + FLType(0.5), dFloorFL, dCeilFL));
                    Udstp[i] = static_cast<T>(Clip((Usrcp[i] - sNeutral) * gain + offset, dFloorFL, dCeilCFL));
                    Vdstp[i] = static_cast<T>(Clip((Vsrcp[i] - sNeutral) * gain + offset, dFloorFL, dCeilCFL));
                }
            }
        }

        vs_aligned_free(idata);
        vs_aligned_free(odata);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif