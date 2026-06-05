#ifndef _AnimationRuntime_h_
#define _AnimationRuntime_h_

#include <string>
#include <vector>
#include "ModelData.h"

class AnimationRuntime
{
public:
    static void BuildBindPosePalette(LPModelData modelData, std::vector<MATRIX>& outPalette);
    static bool BuildAnimationPalette(LPModelData modelData, const std::string& clipName, float timeSeconds, std::vector<MATRIX>& outPalette);
};

#endif //_AnimationRuntime_h_