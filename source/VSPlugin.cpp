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
#include "MSRCR.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi)
{
    vspapi->configPlugin("com.vapoursynth.retinex", "retinex",
        "Implementation of Retinex algorithm for VapourSynth.",
        VS_MAKE_VERSION(5, 0), VAPOURSYNTH_API_VERSION, 0, plugin);

    vspapi->registerFunction("MSRCP", "input:vnode;sigma:float[]:opt;lower_thr:float:opt;upper_thr:float:opt;fulls:int:opt;fulld:int:opt;chroma_protect:float:opt", "clip:vnode", MSRCPCreate, nullptr, plugin);
    vspapi->registerFunction("MSRCR", "input:vnode;sigma:float[]:opt;lower_thr:float:opt;upper_thr:float:opt;fulls:int:opt;fulld:int:opt;restore:float:opt", "clip:vnode", MSRCRCreate, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
