#include "DSP/LivePitchBank.h"
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* message) {if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main() {
    using namespace xiffects;
    for(double sr:{44100.,48000.,96000.}) {
        LivePitchBank live;live.prepare(sr);PitchShifter studio;studio.prepare(sr);
        int peak=-1;
        for(int n=0;n<int(sr*.1);++n) {
            const auto out=live.process({n==0?1.f:0.f,0},1,0,1,1,0,0,0);
            if(out[0]>.5f) peak=n;
            check(out[1]==0,"Live stereo isolation");
        }
        check(peak==live.latency() && peak/sr<.013,"Measured live impulse <13 ms");
        std::cout<<"sr="<<sr<<" live_samples="<<peak<<" live_ms="<<1000*peak/sr<<" studio_ms="<<2000*studio.latency()/sr<<'\n';
        for(float hz:{100.f,146.83f,452.f,880.f,1396.91f}) {
            PitchCorrection correction;correction.prepare(sr);live.prepare(sr);
            std::array<bool,12> notes;notes.fill(true);
            double phase=0,first=-1,last=0;int crossings=0;float previous=0;
            for(int n=0;n<int(sr*.9);++n) {
                const double t=n/sr;
                phase+=2*double(pi)*hz/sr;
                const float in=t<.1?0:float(.2*std::sin(phase)*std::min(1.,(t-.1)/.005));
                const float ratio=correction.process(in,0,0,notes,1,12);
                const float x=live.process({in,in},ratio,1,1,1,0,0,correction.detector.frequency)[0];
                check(std::isfinite(x),"Live finite output");
                if(t>.4 && previous<=0 && x>0) {
                    const double at=n-x/(x-previous);if(first<0)first=at;last=at;++crossings;
                }
                previous=x;
            }
            const double measured=(crossings-1)*sr/(last-first);
            const double target=440*std::exp2((std::round(69+12*std::log2(hz/440))-69)/12);
            std::cout<<"input="<<hz<<" actual="<<measured<<" target="<<target<<'\n';
            check(crossings>2 && std::abs(1200*std::log2(measured/target))<15,"Live corrected fundamental within 15 cents");
        }
        for(float ratio:{.25f,.5f,2.f,4.f}) {
            live.prepare(sr);double first=-1,last=0;int crossings=0;float previous=0;
            for(int n=0;n<int(sr*.6);++n) {
                const float in=.2f*std::sin(2*pi*440*float(n)/float(sr));
                const float x=live.process({in,in},1,0,ratio,1,1,0,440)[0];
                if(n>int(sr*.3) && previous<=0 && x>0) {
                    const double at=n-x/(x-previous);if(first<0)first=at;last=at;++crossings;
                }
                previous=x;
            }
            check(std::abs(1200*std::log2(((crossings-1)*sr/(last-first))/(440*ratio)))<15,"Live octave pitch ratio");
        }
    }
}
