#include "DSP/Effects.h"
#include <cstdlib>
#include <iostream>
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using namespace xiffects;
    for(double sr : {44100.,48000.,96000.}) {
        Compressor comp; comp.prepare(sr);
        float gain=1;
        for(int i=0;i<int(sr);++i) gain=comp.gain(1,-20,4,0);
        check(std::abs(gain-db(-15))<0.001f,"Compressor ratio");
        Delay delay; delay.prepare(sr); float peak=0;
        for(int i=0;i<int(sr);++i) {
            auto out=delay.process({i==0?1.f:0.f,0},0.5f,0);
            if(i>int(sr*0.49) && i<int(sr*0.51)) peak=std::max(peak,out[0]);
            check(out[1]==0,"Delay channel isolation");
        }
        check(peak>0.99f,"Delay impulse time");
        Equalizer eq; eq.prepare(sr); Distortion dist; dist.prepare(sr);
        Phaser phaser; phaser.prepare(sr); Reverb reverb; reverb.prepare(sr);
        double reverbEnergy=0, distortionDifference=0, phaserDifference=0;
        for(int i=0;i<int(sr);++i) {
            const float x=0.2f*std::sin(2*pi*440*float(i)/float(sr));
            check(std::abs(eq.process(x,0,{1,1,1,1})-x)<1.e-6f,"EQ unity");
            const float d=dist.process(x,0,0.5f,0.5f);
            distortionDifference+=std::abs(d-x);
            auto p=phaser.process({x,x},0.6f,0.8f,0.5f);
            phaserDifference+=std::abs(p[0]-x);
            const float r=reverb.process(i==0?1.f:0.f,0,0.5f,2.8f);
            check(std::isfinite(r)&&std::isfinite(p[0])&&std::isfinite(d),"Finite DSP output");
            reverbEnergy+=r*r;
        }
        check(reverbEnergy>0.1,"Reverb has tail");
        check(distortionDifference>1 && phaserDifference>1,"Effects change audio");
    }
    std::array<bool,12> notes; notes.fill(true);
    check(allowed(60,0,1,notes)&&!allowed(61,0,1,notes),"C major mask");
    check(allowed(61,1,1,notes)&&!allowed(62,1,1,notes),"Transposition");
    notes[0]=false; check(!allowed(60,0,0,notes),"Manual exclusion");
    notes.fill(false); check(targetNote(60.3f,0,1,notes)==60.3f,"Empty mask bypass");
    std::cout << "DSP tests passed at 44.1, 48 and 96 kHz\n";
}
