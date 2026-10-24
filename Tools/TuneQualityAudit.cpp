// Standalone DSP evidence only: not a real voice/sax, device or host test.
#include "DSP/LivePitchBank.h"
#include <iostream>
#include <cstdlib>
int main(){using namespace xiffects;int failures=0;
    std::cout<<"sample_rate,input_hz,strength,expected_hz,measured_hz,error_cents\n";
    for(double sr:{44100.,48000.,96000.})for(float hz:{226.f,452.f,904.f})for(float strength:{0.f,.5f,1.f}) {
        PitchCorrection tune;tune.prepare(sr);LivePitchBank bank;bank.prepare(sr);std::array<bool,12> notes;notes.fill(true);
        int crossings=0;double first=0,last=0;float previous=0;
        for(int n=0;n<int(sr);++n){double t=n/sr;float in=t<.1?0:float(.2*std::sin(2*double(pi)*hz*t)*std::min(1.,(t-.1)/.005));
            float ratio=tune.process(in,0,0,notes,strength,12);
            float out=bank.process({in,in},ratio,1,1,1,0,0,tune.detector.frequency)[0];
            if(!std::isfinite(out))return 1;
            if(t>.6 && previous<=0 && out>0){double position=n-out/(out-previous);if(crossings==0)first=position;last=position;++crossings;}previous=out;
        }
        double measured=(crossings-1)*sr/(last-first), full=440*std::exp2((std::round(69+12*std::log2(hz/440))-69)/12);
        double expected=hz*std::pow(full/hz,strength), cents=1200*std::log2(measured/expected);
        std::cout<<sr<<','<<hz<<','<<strength<<','<<expected<<','<<measured<<','<<cents<<'\n';
        if(crossings<2 || std::abs(cents)>5)++failures;
    }
    return failures?1:0;
}
