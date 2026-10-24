#include "Effects.h"
#include <cstdio>
int main(){for(double rate:{44100.,48000.,96000.}){xiffects::Reverb r;r.prepare(rate);for(int i=0;i<int(rate);++i){float in=.1f*std::sin(float(i)*.13f);float size=.2f+float((i/128)%60)*.01f,decay=.5f+float((i/256)%20)*.1f;float out[2]={r.process(in,0,size,decay),r.process(in*.5f,1,size,decay)};std::fwrite(out,sizeof(float),2,stdout);}}}
