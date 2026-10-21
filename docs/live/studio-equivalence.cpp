#include "DSP/Pitch.h"
#include <cstdio>
int main(){for(double sr:{44100.,48000.,96000.}){xiffects::PitchShifter s;s.prepare(sr);for(int n=0;n<int(sr);++n){float x=.2f*std::sin(2*xiffects::pi*440*float(n)/float(sr));float r=n<int(sr/2)?1.f:1.5f;auto y=s.process({x,x*.5f},r,.75f,440);std::fwrite(y.data(),sizeof(float),2,stdout);}}}
