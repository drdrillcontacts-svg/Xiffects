#include "DSP/Effects.h"
#include <cstdlib>
#include <iostream>
void require(bool b){if(!b)std::abort();}
int main(){
    std::array<bool,12> manual;manual.fill(true);
    for(int scale=0;scale<44;++scale) for(int key=0;key<12;++key) {
        require(xiffects::scales[size_t(scale)].mask>0 && xiffects::scales[size_t(scale)].mask<=0xfff);
        for(int n=24;n<108;++n) {
            const bool expected=(xiffects::scales[size_t(scale)].mask&(1u<<((n-key+12)%12)))!=0;
            require(xiffects::allowed(n,key,scale,manual)==expected);
            const float target=xiffects::targetNote(float(n)+.23f,key,scale,manual);
            require(xiffects::allowed(int(target),key,scale,manual));
            manual[size_t(n%12)]=false;require(!xiffects::allowed(n,key,scale,manual));manual.fill(true);
        }
    }
    manual.fill(false);require(xiffects::targetNote(69.25f,0,43,manual)==69.25f);
    manual[9]=true;require(xiffects::targetNote(69.25f,7,43,manual)==69);
    std::cout<<"44 scale entries: all keys, transposition, manual intersection and custom targeting passed\n";
}
