#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace xiffects {
inline constexpr std::array<float,8> delayBeats{{.25f,1.f/3,.5f,.75f,2.f/3,1.f,1.5f,2.f}};
inline constexpr std::array<const char*,8> delayDivisionNames{{"1/16","1/8 T","1/8","1/8 D","1/4 T","1/4","1/4 D","1/2"}};
inline float safeTempo(double bpm) {return std::isfinite(bpm) && bpm>0?float(std::clamp(bpm,30.,300.)):120.f;}
inline float syncedDelaySeconds(double bpm,int division) {return 60.f/safeTempo(bpm)*delayBeats[size_t(std::clamp(division,0,7))];}
}
