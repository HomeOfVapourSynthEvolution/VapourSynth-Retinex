#include "MSRCP.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("AOJIAO_mawen1250->AOwen.Retina_Cortex->Retinex", "retinex",
        "Implementation of Retinex algorithm for VapourSynth.",
        VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("MSRCP", "input:clip;sigma:float[]:opt;lower_thr:float:opt;upper_thr:float:opt;fulls:int:opt;fulld:int:opt", MSRCPCreate, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
