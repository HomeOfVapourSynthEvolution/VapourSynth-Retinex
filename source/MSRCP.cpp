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


#include "MSRCP.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void VS_CC MSRCPInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    MSRCPData *d = reinterpret_cast<MSRCPData *>(*instanceData);

    vsapi->setVideoInfo(d->vi, 1, node);
}

const VSFrameRef *VS_CC MSRCPGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    const MSRCPData *d = reinterpret_cast<MSRCPData *>(*instanceData);

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef *src = vsapi->getFrameFilter(n, d->node, frameCtx);
        const VSFormat *fi = vsapi->getFrameFormat(src);
        int width = vsapi->getFrameWidth(src, 0);
        int height = vsapi->getFrameHeight(src, 0);
        const int planes[] = { 0, 1, 2 };
        const VSFrameRef * cp_planes[] = { d->process[0] ? nullptr : src, d->process[1] ? nullptr : src, d->process[2] ? nullptr : src };
        VSFrameRef *dst = vsapi->newVideoFrame2(fi, width, height, cp_planes, planes, src, core);

        if (d->process[0] == 0) {}
        else if (d->vi->format->bytesPerSample == 1)
        {
            Retinex_MSRCP<uint8_t>(dst, src, vsapi, *d);
        }
        else if (d->vi->format->bytesPerSample == 2)
        {
            Retinex_MSRCP<uint16_t>(dst, src, vsapi, *d);
        }

        vsapi->freeFrame(src);

        return dst;
    }

    return nullptr;
}

void VS_CC MSRCPFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    MSRCPData *d = reinterpret_cast<MSRCPData *>(instanceData);

    delete d;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void VS_CC MSRCPCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    MSRCPData *data = new MSRCPData(vsapi);
    MSRCPData &d = *data;

    int i, m, error;

    d.node = vsapi->propGetNode(in, "input", 0, nullptr);
    d.vi = vsapi->getVideoInfo(d.node);

    if (!d.vi->format)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid input clip, only constant format input supported");
        return;
    }
    if (d.vi->format->sampleType != stInteger || (d.vi->format->bytesPerSample != 1 && d.vi->format->bytesPerSample != 2))
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid input clip, only 8-16 bit int formats supported");
        return;
    }
    if (d.vi->format->subSamplingH || d.vi->format->subSamplingW)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: sub-sampled format is not supported, convert it to YUV444 or RGB first");
        return;
    }

    m = vsapi->propNumElements(in, "sigma");
    if (m > 0)
    {
        for (i = 0; i < m; i++)
        {
            d.sigma.push_back(vsapi->propGetFloat(in, "sigma", i, nullptr));

            if (d.sigma[i] < 0)
            {
                delete data;
                vsapi->setError(out, "retinex.MSRCP: Invalid \"sigma\" assigned, must be non-negative float number");
                return;
            }
        }
    }
    else
    {
        d.sigma.push_back(25);
        d.sigma.push_back(80);
        d.sigma.push_back(250);
    }

    size_t s, scount = d.sigma.size();

    for (s = 0; s < scount; s++)
    {
        if (d.sigma[s] > 0) break;
    }
    if (s >= scount)
    {
        for (i = 0; i < 3; i++)
            d.process[i] = 0;
    }

    d.lower_thr = vsapi->propGetFloat(in, "lower_thr", 0, &error);
    if (error)
        d.lower_thr = 0;
    if (d.lower_thr < 0)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid \"lower_thr\" assigned, must be float number ranges in [0, 1)");
        return;
    }

    d.upper_thr = vsapi->propGetFloat(in, "upper_thr", 0, &error);
    if (error)
        d.upper_thr = 0;
    if (d.upper_thr < 0)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid \"upper_thr\" assigned, must be float number ranges in [0, 1)");
        return;
    }

    if (d.lower_thr + d.upper_thr >= 1)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid \"lower_thr\" and \"upper_thr\" assigned, the sum of which mustn't equal or exceed 1");
        return;
    }

    d.fulls = vsapi->propGetInt(in, "fulls", 0, &error) == 0 ? false : true;
    if (error)
        d.fulls_select();

    d.fulld = vsapi->propGetInt(in, "fulld", 0, &error) == 0 ? false : true;
    if (error)
        d.fulld = d.fulls;

    d.chroma_protect = vsapi->propGetFloat(in, "chroma_protect", 0, &error);
    if (error)
        d.chroma_protect = 1.2;
    if (d.chroma_protect < 1)
    {
        delete data;
        vsapi->setError(out, "retinex.MSRCP: Invalid \"chroma_protect\" assigned, must be float number ranges in [1, +inf)");
        return;
    }

    // Create filter
    vsapi->createFilter(in, out, "MSRCP", MSRCPInit, MSRCPGetFrame, MSRCPFree, fmParallel, 0, data, core);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
