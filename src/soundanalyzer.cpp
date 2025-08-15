//+--------------------------------------------------------------------------
//
// File:        SoundAnalyzer.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Template instantiations for SoundAnalyzer class
//
//---------------------------------------------------------------------------

#include "soundanalyzer.h"

ProjectSoundAnalyzer g_Analyzer;

#if ENABLE_AUDIO

// Explicit template instantiations to ensure proper linkage
template class SoundAnalyzer<kParamsMesmerizer>;
template class SoundAnalyzer<kParamsPCRemote>;
template class SoundAnalyzer<kParamsM5>;
template class SoundAnalyzer<kParamsM5Plus2>;
template class SoundAnalyzer<kParamsI2SExternal>;

#endif

