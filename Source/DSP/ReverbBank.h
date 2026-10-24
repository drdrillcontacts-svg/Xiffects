#pragma once
#include "Effects.h"
namespace xiffects {
// Algorithmic Room (legacy), Hall, Plate-like and Chamber characters.
// All engines keep running: switching crossfades without clearing/cutting tails.
class ReverbBank {
    std::array<Reverb,4> engines;
    std::array<Smoother,4> weights;
public:
    void prepare(double rate,int mode=0) {
        for(size_t i=0;i<4;++i){engines[i].prepare(rate,int(i));weights[i].prepare(rate,int(i)==mode?1.f:0.f,.05f);}
    }
    std::array<float,2> process(std::array<float,2> x,float size,float decay,int mode) {
        mode=std::clamp(mode,0,3);std::array<float,2> out{};
        for(size_t i=0;i<4;++i){float w=weights[i].next(int(i)==mode?1.f:0.f);
            for(size_t c=0;c<2;++c)out[c]+=engines[i].process(x[c],c,size,decay)*w;}
        return out;
    }
};
}
