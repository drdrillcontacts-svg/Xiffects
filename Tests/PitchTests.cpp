#include "DSP/Pitch.h"
#include <cstdlib>
#include <iostream>
#include <random>
void require(bool ok,const char* text) { if(!ok) { std::cerr<<text<<'\n'; std::exit(1); } }
// Independent spectral measurement, not the detector under test.
float dominant(const std::vector<float>& samples,float sr,float expected) {
    double best=-1; float frequency=0;
    for(float f=expected*0.94f; f<expected*1.06f; f+=0.5f) {
        double real=0,imag=0;
        for(size_t i=0;i<samples.size();++i) {
            const double angle=2*double(xiffects::pi)*f*double(i)/sr;
            const double window=0.5-0.5*std::cos(2*double(xiffects::pi)*double(i)/double(samples.size()-1));
            real+=samples[i]*std::cos(angle)*window; imag+=samples[i]*std::sin(angle)*window;
        }
        const double power=real*real+imag*imag;
        if(power>best) {best=power;frequency=f;}
    }
    require(best>1,"Pitch output has energy"); return frequency;
}
int main() {
    using namespace xiffects;
    std::array<bool,12> notes; notes.fill(true);
    for(double sr:{44100.,48000.,96000.}) {
        PitchDetector detector;
        for(float frequency:{146.83f,261.63f,440.f,880.f,1396.91f}) {
            detector.prepare(sr);
            for(int i=0;i<int(sr*0.3);++i) {
                const float phase=2*pi*frequency*float(i)/float(sr);
                detector.push(0.2f*std::sin(phase)+0.1f*std::sin(2*phase)+0.05f*std::sin(3*phase));
            }
            require(std::abs(1200*std::log2(detector.frequency/frequency))<15,"YIN harmonic signal accuracy");
        }
        detector.prepare(sr);
        for(int i=0;i<int(sr);++i) detector.push(0);
        require(detector.frequency==0,"Silence rejected");
        std::mt19937 random(42); std::uniform_real_distribution<float> noise(-0.1f,0.1f);
        for(int i=0;i<int(sr);++i) detector.push(noise(random));
        require(detector.frequency==0,"Unpitched noise rejected");
        PitchShifter shifter;
        shifter.prepare(sr);
        for(int i=0;i<int(sr*0.1);++i) {
            const auto out=shifter.process({i==0?1.f:0.f,0},1,1);
            require(std::abs(out[0]-(i==shifter.latency()?1.f:0.f))<1.e-6f,"Unity impulse and declared latency");
            require(out[1]==0,"Pitch stereo isolation");
        }
        for(float ratio:{0.25f,0.5f,1.0594631f,2.f,4.f}) {
            shifter.prepare(sr); std::vector<float> result;
            for(int i=0;i<int(sr*0.5);++i) {
                const float x=0.2f*std::sin(2*pi*440*float(i)/float(sr));
                const auto out=shifter.process({x,x},ratio,1,440);
                require(std::isfinite(out[0]) && out[0]==out[1],"Finite coherent stereo pitch");
                if(i>int(sr*0.25)) result.push_back(out[0]);
            }
            const float measured=dominant(result,float(sr),440*ratio);
            std::cout << sr << " ratio " << ratio << " measured " << measured << " expected " << 440*ratio << std::endl;
            // Pitch-synchronous windows must preserve the requested fundamental.
            require(std::abs(1200*std::log2(measured/(440*ratio)))<8,"Pitch shift spectral frequency");
        }
        PitchCorrection correction; correction.prepare(sr); float ratio=1;
        const float inputHz=452;
        for(int i=0;i<int(sr*0.6);++i)
            ratio=correction.process(0.2f*std::sin(2*pi*inputHz*float(i)/float(sr)),0,0,notes,1,12);
        require(std::abs(1200*std::log2(inputHz*ratio/440))<8,"Full correction reaches A4");
        notes[9]=false;
        for(int i=0;i<int(sr*0.5);++i)
            ratio=correction.process(0.2f*std::sin(2*pi*inputHz*float(i)/float(sr)),0,0,notes,1,12);
        require(ratio>1,"Manual note exclusion changes actual correction ratio");
        notes.fill(false);
        for(int i=0;i<int(sr*0.3);++i)
            ratio=correction.process(0.2f*std::sin(2*pi*inputHz*float(i)/float(sr)),0,0,notes,1,12);
        require(std::abs(ratio-1)<1.e-4f,"Empty mask returns unity");
        notes.fill(true);
    }
    std::cout<<"Pitch detection, shift, correction and latency tests passed\n";
}
