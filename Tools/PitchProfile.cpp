// Offline measurement only. Never call clocks or collect vectors in a plugin callback.
#include "DSP/Pitch.h"
#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    using namespace xiffects;
    using Clock=std::chrono::steady_clock;
    std::cout<<"kind,sample_rate,block_size,test,samples,milliseconds,p50_us,p99_us,max_us,deadline_misses\n";
    for(double sr:{44100.,48000.,96000.}) {
        PitchShifter first,second;first.prepare(sr);second.prepare(sr);
        int impulse=-1;
        for(int n=0;n<int(sr*0.1);++n) {
            const auto x=second.process(first.process({n==0?1.f:0.f,0},1,0),1,0);
            if(x[0]>0.5f) impulse=n;
        }
        std::cout<<"latency,"<<sr<<",0,two_stage_unity,"<<impulse<<','<<1000*impulse/sr<<",0,0,0,0\n";
        if(impulse!=first.latency()+second.latency()) return 1;
        for(float hz:{146.83f,440.f,880.f}) {
            PitchDetector detector;detector.prepare(sr);int acquisition=-1;
            for(int n=0;n<int(sr*0.25);++n) {
                detector.push(.2f*std::sin(2*pi*hz*float(n)/float(sr)));
                if(detector.frequency>0 && std::abs(1200*std::log2(detector.frequency/hz))<20) {acquisition=n+1;break;}
            }
            std::cout<<"acquisition,"<<sr<<",0,sine_"<<hz<<','<<acquisition<<','<<1000*acquisition/sr<<",0,0,0,0\n";
            if(acquisition<0) return 1;
        }
        for(int block:{32,64,128,256}) {
            PitchCorrection correction;correction.prepare(sr);
            PitchShifter tune,octave,pitch;tune.prepare(sr);octave.prepare(sr);pitch.prepare(sr);
            const int blocks=int(sr*2)/block;
            std::vector<float> input(size_t((blocks+64)*block));
            for(size_t i=0;i<input.size();++i) input[i]=.2f*std::sin(2*pi*452*float(i)/float(sr));
            std::vector<double> times;times.reserve(size_t(blocks));
            std::array<bool,12> notes;notes.fill(true);
            volatile float sink=0;
            int missed=0;
            for(int b=0;b<blocks+64;++b) {
                const auto begin=Clock::now();
                for(int n=0;n<block;++n) {
                    const float in=input[size_t(b*block+n)];
                    const float ratio=correction.process(in,0,0,notes,1,12);
                    const auto x=tune.process({in,in},ratio,1,correction.detector.frequency);
                    const auto a=octave.process(x,0.5f,0.35f,correction.detector.frequency*ratio);
                    const auto p=pitch.process(x,1.498307f,0.3f,correction.detector.frequency*ratio);
                    sink=a[0]+p[0];
                }
                const double us=std::chrono::duration<double,std::micro>(Clock::now()-begin).count();
                if(b>=64) {times.push_back(us);if(us>1.e6*block/sr) ++missed;}
            }
            if(!std::isfinite(sink)) return 1;
            std::sort(times.begin(),times.end());
            std::cout<<"callback,"<<sr<<','<<block<<",pitch_subsystem_only,0,0,"
                <<times[times.size()/2]<<','<<times[(times.size()-1)*99/100]<<','<<times.back()<<','<<missed<<'\n';
        }
    }
}
