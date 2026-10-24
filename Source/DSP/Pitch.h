#pragma once
#include "Effects.h"
#include "Interpolation.h"

namespace xiffects {
// Bounded-cost YIN analysis at approximately 12 kHz. Two cascaded low-pass
// sections precede decimation. Analysis is monophonic, 100–1600 Hz.
class PitchDetector {
public:
    void prepare(double sampleRate) {
        decimation = std::max(1, int(sampleRate / 12000.0));
        rate = float(sampleRate / decimation);
        coefficient = 1 - std::exp(-2*pi*2500/float(sampleRate));
        ring.fill(0); difference.fill(0); rawDifference.fill(0); write=filled=counter=hop=0;
        low1=low2=frequency=confidence=0;
    }
    bool push(float input) {
        low1 += coefficient*(input-low1); low2 += coefficient*(low1-low2);
        if (++counter < decimation) return false;
        counter=0; ring[write]=low2; write=(write+1)%ring.size();
        filled=std::min(filled+1,ring.size());
        if (++hop < 128) return false;
        hop=0;
        if (filled < ring.size()) return false;
        analyse(); return true;
    }
    float frequency=0, confidence=0;
private:
    void analyse() {
        std::array<float,512> frame{};
        float energy=0;
        for(size_t i=0;i<frame.size();++i) { frame[i]=ring[(write+i)%ring.size()]; energy+=frame[i]*frame[i]; }
        frequency=confidence=0;
        if (energy/512 < 1.e-7f) return;
        const int maximum=std::min(200,int(rate/100));
        const int minimum=std::max(2,int(rate/1600));
        float running=0;
        difference[0]=1;
        for(int lag=1;lag<=maximum;++lag) {
            float sum=0;
            for(size_t i=0;i<256;++i) { const float d=frame[i]-frame[i+size_t(lag)]; sum+=d*d; }
            rawDifference[size_t(lag)]=sum;
            running+=sum;
            difference[size_t(lag)]=running>1.e-12f?sum*float(lag)/running:1;
        }
        for(int lag=minimum;lag<maximum-1;++lag) {
            if (difference[size_t(lag)] >= 0.15f) continue;
            while(lag+1<maximum && difference[size_t(lag+1)]<difference[size_t(lag)]) ++lag;
            if(lag>=maximum) return;
            // CMND selects the period, but its lag-dependent normalisation biases
            // parabolic refinement at short periods. Refine the raw difference
            // minimum instead; keep confidence based on the normalised curve.
            const float normalized=difference[size_t(lag)];
            const float a=rawDifference[size_t(lag-1)], b=rawDifference[size_t(lag)], c=rawDifference[size_t(lag+1)];
            const float denominator=a-2*b+c;
            const float offset=std::abs(denominator)>1.e-9f?std::clamp(0.5f*(a-c)/denominator,-0.5f,0.5f):0;
            const float candidate=rate/(float(lag)+offset);
            // Lag rounding/interpolation can otherwise publish values above the
            // documented range (e.g. a 1700 Hz input at 48 kHz). Do not fall back
            // to a subharmonic just to find an in-range candidate.
            // A 15-cent estimator guard preserves genuine boundary tones despite
            // parabolic interpolation bias. Published values stay in range.
            constexpr float edgeGuard=1.008701984f; // 2^(15/1200)
            if(candidate>=100/edgeGuard && candidate<=1600*edgeGuard) {
                confidence=1-normalized; frequency=std::clamp(candidate,100.f,1600.f);
            }
            return;
        }
    }
    std::array<float,512> ring{};
    std::array<float,201> difference{}, rawDifference{};
    size_t write=0, filled=0;
    int decimation=4, counter=0, hop=0;
    float rate=12000, coefficient=0, low1=0, low2=0;
};

// Two Hann-windowed moving read heads. Both channels share phase, preserving
// stereo coherence. Fixed dry delay equals the mean wet delay; variable-delay
// pitch processing is not phase-identical to dry at non-unity ratios.
class PitchShifter {
public:
    void prepare(double sampleRate,double windowSeconds=0.04,float maximumRatio=4.f,bool cubicInterpolation=false) {
        ratioLimit=maximumRatio; useCubic=cubicInterpolation;
        window=std::max(64,int(sampleRate*windowSeconds));
        if(window%2) ++window;
        for(auto& channel:memory) channel.assign(size_t(window*2+8),0);
        position=0; phase=0.5f; sampleRateHz=float(sampleRate); unity.prepare(sampleRate,0,0.01f);
        grain.prepare(sampleRate,float(window),0.02f);
    }
    int latency() const { return window*3/4+2; }
    std::array<float,2> process(std::array<float,2> input, float ratio, float mix, float fundamental=0) {
        ratio=std::clamp(ratio,1.f/ratioLimit,ratioLimit);
        const float active=unity.next(std::abs(ratio-1)>1.e-5f?1.f:0.f);
        float targetWindow=float(window);
        if(fundamental>=100 && fundamental<=6400) {
            const float period=sampleRateHz/fundamental;
            const float cycles=std::max(2.f,2*std::round(float(window)/(2*period)));
            targetWindow=std::clamp(cycles*period,float(window)*0.5f,float(window)*1.5f);
        }
        const float length=grain.next(targetWindow);
        phase+=(1-ratio)/length; phase-=std::floor(phase);
        const float other=phase<0.5f?phase+0.5f:phase-0.5f;
        const float weight=0.5f-0.5f*std::cos(2*pi*phase);
        std::array<float,2> output{};
        for(size_t c=0;c<2;++c) {
            memory[c][position]=input[c];
            const float dry=read(c,float(latency()));
            dryOutput[c]=dry;
            const float wet=read(c,float(latency())+(phase-0.5f)*length)*weight + read(c,float(latency())+(other-0.5f)*length)*(1-weight);
            output[c]=dry+(wet-dry)*std::clamp(mix,0.f,1.f)*active;
        }
        position=(position+1)%memory[0].size(); return output;
    }
    std::array<float,2> dryOutput{};
private:
    float read(size_t channel,float delay) const {
        float index=float(position)-delay;
        if(index<0) index+=float(memory[channel].size());
        const auto i=size_t(index); const float fraction=index-float(i);
        if(useCubic) {
            if(fraction==0)return memory[channel][i];
            const auto size=memory[channel].size();
            const auto previous=i==0?size-1:i-1, next=i+1<size?i+1:0, after=next+1<size?next+1:0;
            return cubicInterpolate(memory[channel][previous],memory[channel][i],memory[channel][next],memory[channel][after],fraction);
        }
        return memory[channel][i]*(1-fraction)+memory[channel][(i+1)%memory[channel].size()]*fraction;
    }
    std::array<std::vector<float>,2> memory;
    size_t position=0;
    int window=1920;
    float phase=0.5f;
    Smoother unity, grain;
    float sampleRateHz=48000,ratioLimit=4.f;
    bool useCubic=false;
};

class PitchCorrection {
public:
    void prepare(double sampleRate) {
        detector.prepare(sampleRate); rate=float(sampleRate);
        offset=0; detected=corrected=0; lastTarget=-1000;
        voicedFrames=0;
        targetValid=false; cachedDetected=0; cachedKey=cachedScale=0;
        cachedNotes.fill(false); targetOffset=0;
        cachedMilliseconds=-1; retuneCoefficient=0;
    }
    // Called on unprocessed input; correction runs before octaving/harmonisation.
    float process(float sample,int key,int scale,const std::array<bool,12>& notes,
                  float strength,float milliseconds) {
        if(detector.push(sample)) {
            detected=detector.frequency>0?69+12*std::log2(detector.frequency/440):0;
            // A partial onset window can initially cross a note boundary. Keep
            // the first three voiced analysis frames provisional: correct now,
            // but do not let hysteresis latch the first estimate permanently.
            // No buffering, muting or extra delay is introduced.
            voicedFrames=detected>0?std::min(voicedFrames+1,4):0;
            if(voicedFrames<=3) targetValid=false;
        }
        // Target selection changes only on a new pitch value or musical map.
        // Strength remains sample-accurate and does not invalidate the target.
        if(!targetValid || detected!=cachedDetected || key!=cachedKey || scale!=cachedScale || notes!=cachedNotes) {
            cachedDetected=detected;cachedKey=key;cachedScale=scale;cachedNotes=notes;targetValid=true;
            targetOffset=0;
            if(detected>0) {
                float next=targetNote(detected,key,scale,notes);
                // 15-cent hysteresis, but never retain an excluded target.
                if(voicedFrames>3 && lastTarget>-999 && allowed(int(lastTarget),key,scale,notes)
                   && std::abs(detected-lastTarget)<std::abs(detected-next)+0.15f) next=lastTarget;
                lastTarget=std::abs(next-std::round(next))<1.e-4f?next:-1000;
                targetOffset=std::clamp(next-detected,-12.f,12.f);
            } else lastTarget=-1000;
        }
        const float desired=targetOffset*std::clamp(strength,0.f,1.f);
        const float safeMilliseconds=std::max(milliseconds,1.f);
        if(safeMilliseconds!=cachedMilliseconds) {
            cachedMilliseconds=safeMilliseconds;
            retuneCoefficient=std::exp(-1/(rate*safeMilliseconds*0.001f));
        }
        offset=desired+retuneCoefficient*(offset-desired);
        corrected=detected>0?detected+offset:0;
        return std::exp2(offset/12);
    }
    float detected=0, corrected=0;
    PitchDetector detector;
private:
    float rate=48000, offset=0, lastTarget=-1000;
    int voicedFrames=0;
    bool targetValid=false;
    float cachedDetected=0, targetOffset=0, cachedMilliseconds=-1, retuneCoefficient=0;
    int cachedKey=0,cachedScale=0;
    std::array<bool,12> cachedNotes{};
};
}
