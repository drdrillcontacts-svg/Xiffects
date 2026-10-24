#pragma once
#include <algorithm>
#include "Scales.h"
#include <array>
#include <cmath>
#include <vector>

namespace xiffects {
constexpr float pi = 3.14159265358979323846f;
inline float db(float value) { return std::pow(10.0f, value / 20.0f); }
struct Smoother {
    float value = 0, coefficient = 0;
    void prepare(double sr, float initial, float seconds = 0.02f) {
        value = initial; coefficient = std::exp(-1.0f / (float(sr) * seconds));
    }
    float next(float target) { return value = target + coefficient * (value - target); }
};
struct Compressor {
    float envelope = 0, attack = 0, release = 0;
    void prepare(double sr) {
        envelope = 0; attack = std::exp(-1.0f / float(sr * 0.01));
        release = std::exp(-1.0f / float(sr * 0.12));
    }
    float gain(float peak, float threshold, float ratio, float makeup) {
        const float c = peak > envelope ? attack : release;
        envelope = peak + c * (envelope - peak);
        const float above = std::max(0.0f, 20.0f * std::log10(std::max(envelope, 1.e-9f)) - threshold);
        return db(makeup - above * (1.0f - 1.0f / ratio));
    }
};
class Delay {
    std::array<std::vector<float>, 2> memory;
    size_t position = 0;
    double sampleRate = 48000;
    Smoother time;
public:
    void prepare(double sr, double maximumSeconds=2.1) {
        sampleRate = sr; position = 0;
        for (auto& m : memory) m.assign(size_t(sr * maximumSeconds) + 2, 0);
        time.prepare(sr, float(sr * 0.5), 0.08f);
    }
    std::array<float, 2> process(std::array<float, 2> in, float seconds, float feedback) {
        const float delay = time.next(float(sampleRate) * seconds);
        const auto size = memory[0].size();
        float read = float(position) - std::clamp(delay, 1.0f, float(size - 2));
        if (read < 0) read += float(size);
        const auto index = size_t(read); const float frac = read - float(index);
        std::array<float, 2> out{};
        for (size_t c = 0; c < 2; ++c) {
            out[c] = memory[c][index] * (1-frac) + memory[c][(index+1)%size] * frac;
            memory[c][position] = in[c] + out[c] * std::clamp(feedback, 0.0f, 0.95f);
        }
        position = (position + 1) % size; return out;
    }
};
struct Distortion {
    std::array<float, 2> low{}, previous{};
    float sr = 48000;
    void prepare(double rate) { sr = float(rate); low.fill(0); previous.fill(0); }
    float process(float in, size_t channel, float drive, float tone) {
        const float amount = 1 + drive * 30;
        // First-order antiderivative anti-aliasing for the tanh waveshaper.
        // Stable log(cosh(x)) avoids overflow at large input levels.
        const auto integral=[](float x) {
            const float a=std::abs(x);return a+std::log1p(std::exp(-2*a))-0.69314718056f;
        };
        const float current=in*amount, delta=current-previous[channel];
        const float shaped=(std::abs(delta)>1.e-3f?
            (integral(current)-integral(previous[channel]))/delta:
            std::tanh((current+previous[channel])*0.5f))/std::sqrt(amount);
        previous[channel]=current;
        const float coefficient = 1 - std::exp(-2*pi*(400 + tone*14000)/sr);
        low[channel] += coefficient * (shaped - low[channel]); return low[channel];
    }
};
// Complementary one-pole bands: exactly transparent when all gains are zero.
struct Equalizer {
    std::array<std::array<float, 3>, 2> low{};
    std::array<float, 3> coefficient{};
    void prepare(double sr) {
        low = {}; size_t i = 0;
        for (float hz : {200.0f, 1800.0f, 6500.0f})
            coefficient[i++] = 1 - std::exp(-2*pi*hz/float(sr));
    }
    float process(float in, size_t c, const std::array<float, 4>& gains) {
        for (size_t i=0; i<3; ++i) low[c][i] += coefficient[i]*(in-low[c][i]);
        return low[c][0]*gains[0] + (low[c][1]-low[c][0])*gains[1]
            + (low[c][2]-low[c][1])*gains[2] + (in-low[c][2])*gains[3];
    }
};
struct Phaser {
    std::array<std::array<float, 6>, 2> state{};
    std::array<float, 2> feedbackState{};
    float phase = 0, sr = 48000;
    void prepare(double rate) { sr=float(rate); phase=0; state={}; feedbackState={}; }
    std::array<float, 2> process(std::array<float, 2> in, float rate, float depth, float feedback) {
        phase += 2*pi*rate/sr; if (phase > 2*pi) phase -= 2*pi;
        const float hz = 300 + 1800*(0.5f+0.5f*std::sin(phase));
        const float t = std::tan(pi*hz/sr), a = (1-t)/(1+t);
        for (size_t c=0;c<2;++c) {
            float y = in[c] + feedbackState[c]*feedback*0.8f;
            for (auto& z : state[c]) { const float v = -a*y+z; z=y+a*v; y=v; }
            feedbackState[c]=y;
            in[c] = in[c]*(1-depth*0.5f) + y*depth*0.5f;
        }
        return in;
    }
};
// Four parallel damped combs per channel; decay specifies approximate low-frequency RT60.
class Reverb {
    struct Comb { std::vector<float> data; size_t pos=0; float damp=0, cachedLength=-1, cachedDecay=-1, feedback=0; };
    std::array<std::array<Comb, 4>, 2> combs;
    float sr=48000, damping=.35f;
    struct Allpass { std::vector<float> data; size_t pos=0;
        float process(float x) {const float delayed=data[pos];const float y=delayed-.5f*x;data[pos]=x+.5f*y;pos=(pos+1)%data.size();return y;}
    };
    std::array<std::array<Allpass,4>,2> diffusion;
    int mode=0;
public:
    void prepare(double rate,int character=0) {
        mode=std::clamp(character,0,3);sr=float(rate);
        const float lengthScale=std::array<float,4>{{1.f,1.6f,.8f,1.1f}}[size_t(mode)];
        damping=std::array<float,4>{{.35f,.24f,.58f,.18f}}[size_t(mode)];
        for(size_t c=0;c<2;++c)for(size_t i=0;i<4;++i){auto& a=diffusion[c][i];a.data.assign(size_t(sr*(.0023+.0017*double(i)+.00031*double(c))*lengthScale)+1,0);a.pos=0;}
        for (size_t c=0;c<2;++c) for(size_t i=0;i<4;++i) {
            auto& comb=combs[c][i]; comb.data.assign(size_t(sr*(0.031+0.011*double(i)+0.0013*double(c))*lengthScale),0);
            comb.pos=0; comb.damp=0;comb.cachedLength=comb.cachedDecay=-1;
        }
    }
    float process(float in, size_t c, float size, float decay) {
        float out=0;size=std::clamp(size,0.f,1.f);decay=std::clamp(decay,.1f,10.f);
        if(mode!=0)for(auto& a:diffusion[c])in=a.process(in);
        for(auto& comb:combs[c]) {
            const float length=float(comb.data.size()-1)*(0.35f+size*0.65f);
            float read=float(comb.pos)-length;
            if(read<0) read+=float(comb.data.size());
            const auto index=size_t(read); const float fraction=read-float(index);
            const float y=comb.data[index]*(1-fraction)+comb.data[(index+1)%comb.data.size()]*fraction;
            comb.damp += damping*(y-comb.damp);
            if(length!=comb.cachedLength || decay!=comb.cachedDecay){comb.feedback=std::pow(0.001f,length/(sr*decay));comb.cachedLength=length;comb.cachedDecay=decay;}
            comb.data[comb.pos]=in + comb.feedback*comb.damp;
            comb.pos=(comb.pos+1)%comb.data.size(); out+=y*0.25f;
        }
        return out;
    }
};
// Scale mask AND manual switches. No allowed notes means no correction.
inline bool allowed(int midi, int key, int scale, const std::array<bool,12>& manual) {
    const int pc=(midi%12+12)%12, relative=(pc-key+12)%12;
    return manual[size_t(pc)] && (scales[size_t(std::clamp(scale,0,43))].mask & (1u<<relative)) != 0;
}
inline float targetNote(float midi, int key, int scale, const std::array<bool,12>& manual) {
    float target=midi, distance=1000;
    for(int n=int(std::floor(midi))-12;n<=int(std::ceil(midi))+12;++n)
        if(allowed(n,key,scale,manual) && std::abs(float(n)-midi)<distance) {
            target=float(n); distance=std::abs(target-midi);
        }
    return target;
}
}
