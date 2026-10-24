#include "DSP/Pitch.h"
#include <iostream>
#include <vector>
#include <cstdlib>

void check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
struct Result {float corrected=0, measured=0, jump=0;};
Result render(double sr,float hz,float ramp,float silence,bool harmonic=false) {
    xiffects::PitchCorrection correction;correction.prepare(sr);
    xiffects::PitchShifter shifter;shifter.prepare(sr);
    std::array<bool,12> notes;notes.fill(true);
    double phase=0;float previous=0, maxJump=0;std::vector<double> crossings;
    for(int n=0;n<int(sr*(silence+0.85));++n) {
        const double t=double(n)/sr, elapsed=t-silence;
        phase+=2*double(xiffects::pi)*hz/sr;
        float input=elapsed<0?0:float(.2*std::sin(phase)*(std::min(1.0,elapsed/ramp)));
        if(harmonic && elapsed>=0) input+=float(.06*std::sin(2*phase)*std::min(1.0,elapsed/ramp));
        input=std::round(input*32767)/32767; // Same PCM16 quantisation as fixture generator.
        const float ratio=correction.process(input,0,0,notes,1,12);
        const float output=shifter.process({input,input},ratio,1,correction.detector.frequency)[0];
        check(std::isfinite(output),"Finite onset output");
        maxJump=std::max(maxJump,std::abs(output-previous));
        if(elapsed>.35) {
            if(previous<=0 && output>0) crossings.push_back(double(n)-output/(output-previous));
        }
        previous=output;
    }
    check(crossings.size()>2,"Output has periodic energy");
    return {correction.corrected,float((crossings.size()-1)*sr/(crossings.back()-crossings.front())),maxJump};
}
int main(int argc,char**) {
    bool bug=false;
    for(double sr:{44100.,48000.,96000.}) {
        auto result=render(sr,452,.005f,.1f);
        std::cout<<sr<<",452Hz,onset5ms,applied="<<result.corrected<<",audio="<<result.measured<<",jump="<<result.jump<<std::endl;
        if(std::abs(result.corrected-69)>.02f || std::abs(1200*std::log2(result.measured/440))>10) bug=true;
    }
    if(argc>1) return bug?1:0; // Baseline reproducer: nonzero confirms the bug, not a pass.
    check(!bug,"452 Hz onset must settle to A4, not the adjacent note");
    for(double sr:{44100.,48000.,96000.}) for(float ramp:{.001f,.005f,.02f,.05f})
        for(float silence:{0.f,.1f}) for(bool harmonic:{false,true}) {
            auto a=render(sr,452,ramp,silence,harmonic);
            check(std::abs(a.corrected-69)<.02f,"A4 target across onset envelope matrix");
            check(std::abs(1200*std::log2(a.measured/440))<10,"Actual resynthesized audio reaches A4");
            auto b=render(sr,454,ramp,silence,harmonic);
            check(std::abs(b.corrected-70)<.02f,"Above-boundary pitch must select Bb4, not force A4");
        }
    // Repeated voiced onsets must reacquire; established-note boundary vibrato
    // must retain hysteresis. Reset/empty/manual masks remain respected.
    for(double sr:{44100.,48000.,96000.}) {
        xiffects::PitchCorrection c;c.prepare(sr);
        std::array<bool,12> notes;notes.fill(true);double phase=0;
        auto feed=[&](float hz,double seconds) {
            for(int n=0;n<int(sr*seconds);++n) {
                phase+=2*double(xiffects::pi)*hz/sr;
                c.process(hz==0?0:float(.2*std::sin(phase)),0,0,notes,1,12);
            }
        };
        feed(454,.3);feed(0,.2);feed(452,.3);
        check(std::abs(c.corrected-69)<.02f,"Silence re-arms onset acquisition");
        for(int n=0;n<int(sr*.5);++n) {
            const float midi=69.5f+.02f*std::sin(2*xiffects::pi*5*float(n)/float(sr));
            const float hz=440*std::exp2((midi-69)/12);
            phase+=2*double(xiffects::pi)*hz/sr;
            c.process(float(.2*std::sin(phase)),0,0,notes,1,12);
            if(n>int(sr*.1)) check(std::abs(c.corrected-69)<.1f,"Steady boundary vibrato retains target hysteresis");
        }
        notes[9]=false;feed(452,.3);
        check(std::abs(c.corrected-70)<.02f,"Manual exclusion overrides established target");
        notes.fill(false);feed(452,.3);
        check(std::abs(c.corrected-c.detected)<.001f,"Empty mask releases correction");
    }
    std::cout<<"Onset envelope, reacquisition, hysteresis and mask checks passed\n";
}
