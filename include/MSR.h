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
    int fulls = 1;
    int fulld = fulls;

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
            fulls = 0;
        else if (vi->format->colorFamily == cmRGB || vi->format->colorFamily == cmYCoCg)
            fulls = 1;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int Retinex_MSR(FLType *odata, const FLType *idata, const MSRData &d, int height, int width, int stride);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif