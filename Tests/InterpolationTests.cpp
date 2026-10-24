#include "DSP/Interpolation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main() {
    constexpr double pi=3.141592653589793;
    for(double sr:{44100.,48000.,96000.})for(double hz:{452.,4000.,8000.}) {
        double linear=0,cubic=0;
        for(int n=0;n<4096;++n)for(float t:{.125f,.25f,.5f,.75f,.875f}) {
            auto sample=[&](double at){return float(std::sin(2*pi*hz*at/sr));};
            double desired=sample(n+t),a=sample(n)*(1-t)+sample(n+1)*t,b=xiffects::cubicInterpolate(sample(n-1),sample(n),sample(n+1),sample(n+2),t);
            linear+=(a-desired)*(a-desired);cubic+=(b-desired)*(b-desired);
        }
        if(!(cubic<linear))throw std::runtime_error("Cubic fractional delay must reduce these tone errors");
        std::cout<<"INTERPOLATION sr="<<sr<<" hz="<<hz<<" linear_rms="<<std::sqrt(linear/20480)<<" cubic_rms="<<std::sqrt(cubic/20480)<<'\n';
    }
}
