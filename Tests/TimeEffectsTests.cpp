#include "DSP/TempoDelay.h"
#include "DSP/ReverbBank.h"
#include <iostream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("Time-effect regression");}
int main(){
    using namespace xiffects;
    check(std::abs(syncedDelaySeconds(120,5)-.5f)<1e-6);
    check(std::abs(syncedDelaySeconds(60,3)-.75f)<1e-6);
    check(std::abs(syncedDelaySeconds(120,1)-1.f/6)<1e-6);
    check(syncedDelaySeconds(30,7)==4.f);check(safeTempo(NAN)==120);check(safeTempo(0)==120);
    for(double rate:{44100.,48000.,96000.}) {
        std::array<double,4> signatures{};
        for(int mode=0;mode<4;++mode){ReverbBank bank;bank.prepare(rate,mode);Reverb legacy;legacy.prepare(rate);
            double energy=0,late=0;
            for(int n=0;n<int(rate*3);++n){float in=n==0?.2f:0;auto out=bank.process({in,0},.5f,.5f,mode);
                check(std::isfinite(out[0])&&std::abs(out[0])<1);check(out[1]==0);
                energy+=out[0]*out[0];if(n>rate*2)late+=out[0]*out[0];
                signatures[size_t(mode)]+=std::abs(out[0])*(1+n%997);
                if(mode==0)check(out[0]==legacy.process(in,0,.5f,.5f));
            }check(energy>1e-8 && late<energy*.01);
        }
        for(int a=0;a<4;++a)for(int b=a+1;b<4;++b)check(std::abs(signatures[a]-signatures[b])>1e-3);
        ReverbBank switching;switching.prepare(rate);
        for(int n=0;n<int(rate);++n){auto out=switching.process({.1f,.1f},.5f,3,(n/128)%4);check(std::isfinite(out[0])&&std::abs(out[0])<2);}
        Delay delay;delay.prepare(rate,4.1);int first=-1;
        for(int n=0;n<int(rate*9);++n){auto y=delay.process({n==int(rate*5)?1.f:0,0},4,.0f);if(y[0]>.5f)first=n;}
        check(first>=int(rate*9)-int(rate*.002)-1);
    }
    std::cout<<"Tempo math, four distinct stable reverb responses, Room compatibility, switching and 4-second buffer passed\n";
}
