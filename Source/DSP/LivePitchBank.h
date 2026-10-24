#pragma once
#include "Pitch.h"

namespace xiffects {
// Experimental low-latency topology: all voices read the SAME input, not
// correction -> harmonisers in series. One nominal delay for the whole bank.
class LivePitchBank {
public:
    void prepare(double sampleRate) {
        for(auto* voice:{&tune,&octave,&pitch}) voice->prepare(sampleRate,0.016,8.f);
    }
    int latency() const {return tune.latency();}
    std::array<float,2> process(std::array<float,2> input,float correctionRatio,
        float tuneMix,float octaveRatio,float pitchRatio,float octaveMix,float pitchMix,float fundamental) {
        tuneMix=std::clamp(tuneMix,0.f,1.f);
        const auto corrected=tune.process(input,correctionRatio,tuneMix,fundamental);
        // At partial Tune Mix, harmonies interpolate correction in semitones;
        // this is intentionally not the Studio dry/wet cascade's timbre.
        const float harmonyCorrection=std::pow(correctionRatio,tuneMix);
        const auto octaved=octave.process(input,octaveRatio*harmonyCorrection,1,fundamental);
        const auto pitched=pitch.process(input,pitchRatio*harmonyCorrection,1,fundamental);
        const float normalizer=std::max(1.f,octaveMix+pitchMix);
        const float a=octaveMix/normalizer,b=pitchMix/normalizer;
        std::array<float,2> out{};
        for(size_t c=0;c<2;++c) out[c]=corrected[c]*(1-a-b)+octaved[c]*a+pitched[c]*b;
        return out;
    }
private:
    PitchShifter tune,octave,pitch;
};
}
