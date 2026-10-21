#include "DSP/Pitch.h"
#include <cstdlib>
#include <iostream>

void require(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
// Frozen pre-cache control algorithm: preserve sample-wise automation semantics.
// Same detector deliberately isolates the target/coefficient caching change.
class ReferenceCorrection {
public:
    void prepare(double sr) {detector.prepare(sr);rate=float(sr);offset=detected=corrected=0;lastTarget=-1000;}
    float process(float input,int key,int scale,const std::array<bool,12>& notes,float strength,float ms) {
        using namespace xiffects;
        if(detector.push(input)) detected=detector.frequency>0?69+12*std::log2(detector.frequency/440):0;
        float desired=0;
        if(detected>0) {
            float next=targetNote(detected,key,scale,notes);
            if(lastTarget>-999 && allowed(int(lastTarget),key,scale,notes)
                && std::abs(detected-lastTarget)<std::abs(detected-next)+0.15f) next=lastTarget;
            lastTarget=std::abs(next-std::round(next))<1.e-4f?next:-1000;
            desired=std::clamp(next-detected,-12.f,12.f)*std::clamp(strength,0.f,1.f);
        } else lastTarget=-1000;
        const float coefficient=std::exp(-1/(rate*std::max(ms,1.f)*0.001f));
        offset=desired+coefficient*(offset-desired);
        corrected=detected>0?detected+offset:0;
        return std::exp2(offset/12);
    }
    float detected=0,corrected=0;
private:
    xiffects::PitchDetector detector;
    float rate=48000,offset=0,lastTarget=-1000;
};
int main() {
    using namespace xiffects;
    for(double sr:{44100.,48000.,96000.}) {
        PitchDetector detector;detector.prepare(sr);
        for(int i=0;i<int(sr*.3);++i) detector.push(.2f*std::sin(2*pi*1700*float(i)/float(sr)));
        require(detector.frequency==0,"Out-of-range 1700 Hz must not be published as a valid 100-1600 Hz estimate");
        for(float hz:{100.f,1550.f,1600.f}) {
            detector.prepare(sr);
            for(int i=0;i<int(sr*.3);++i) detector.push(.2f*std::sin(2*pi*hz*float(i)/float(sr)));
            require(detector.frequency>=100 && detector.frequency<=1600,"Published pitch stays in range at boundary tones");
            require(std::abs(1200*std::log2(detector.frequency/hz))<15,"Bounds fix must preserve valid boundary tones");
        }
        PitchCorrection actual;ReferenceCorrection reference;actual.prepare(sr);reference.prepare(sr);
        std::array<bool,12> notes;notes.fill(true);
        for(int i=0;i<int(sr*1.5);++i) {
            const int phase=i/997;
            const int key=phase%12,scale=(phase/2)%8;
            if(i%997==0) {
                notes.fill(true);
                notes[size_t(phase%12)]=false;
                if(phase%7==0) notes.fill(false);
            }
            const float strength=float(i%101)*0.01f;
            const float ms=i<int(sr*.75)?12.f:1+float(i%100);
            const float input=(i%5000<500)?0:.2f*std::sin(2*pi*452*float(i)/float(sr));
            const float a=actual.process(input,key,scale,notes,strength,ms);
            const float b=reference.process(input,key,scale,notes,strength,ms);
            require(std::abs(a-b)<1.e-6f,"Cached correction matches reference through automation and empty masks");
            require(std::abs(actual.corrected-reference.corrected)<1.e-5f,"Applied pitch telemetry preserved");
        }
        // Reprepare must invalidate every cache, including same-valued parameters.
        actual.prepare(sr);reference.prepare(sr);notes.fill(true);
        for(int i=0;i<int(sr*.1);++i) {
            const float input=.2f*std::sin(2*pi*452*float(i)/float(sr));
            require(std::abs(actual.process(input,0,0,notes,1,12)-reference.process(input,0,0,notes,1,12))<1.e-6f,"Reprepare resets caches");
        }
    }
    std::cout<<"Detector bounds and sample-wise correction regression passed\n";
}
