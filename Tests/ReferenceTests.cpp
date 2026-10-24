// Product-level offline regression: requires JUCE, not a portable DSP substitute.
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
void set(XiffectsAudioProcessor& p,const char* id,float value) {
    auto* parameter=p.parameters.getParameter(id);require(parameter!=nullptr,"Parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
juce::AudioBuffer<float> render(double rate,int channels,const std::vector<int>& sizes,juce::MemoryBlock* restore=nullptr) {
    XiffectsAudioProcessor processor;
    auto layout=processor.getBusesLayout();
    const auto channelSet=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
    layout.inputBuses.set(0,channelSet);layout.outputBuses.set(0,channelSet);
    require(processor.setBusesLayout(layout),"Layout accepted");
    if(restore) processor.setStateInformation(restore->getData(),int(restore->getSize()));
    else {
        for(const auto* id:{"POWER","OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON"}) set(processor,id,1);
        set(processor,"TUNE_STRENGTH",100);set(processor,"TUNE_MIX",100);
    }
    processor.prepareToPlay(rate,64);
    const int count=int(rate*0.4);
    juce::AudioBuffer<float> result(channels,count);
    int position=0;size_t blockIndex=0;juce::MidiBuffer midi;
    while(position<count) {
        const int length=std::min(sizes[blockIndex++%sizes.size()],count-position);
        juce::AudioBuffer<float> block(channels,length);
        for(int c=0;c<channels;++c) for(int i=0;i<length;++i)
            block.setSample(c,i,0.1f*std::sin(2*xiffects::pi*(c==0?452.f:660.f)*float(position+i)/float(rate)));
        processor.processBlock(block,midi);
        for(int c=0;c<channels;++c) {
            for(int i=0;i<length;++i) require(std::isfinite(block.getSample(c,i)),"Finite active-chain output");
            result.copyFrom(c,position,block,c,0,length);
        }
        position+=length;
    }
    return result;
}
void compare(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b) {
    for(int c=0;c<a.getNumChannels();++c) for(int n=0;n<a.getNumSamples();++n)
        require(std::abs(a.getSample(c,n)-b.getSample(c,n))<=2.e-6f,"Block-size invariant active Reference audio");
}
int main() {
    try {
        juce::ScopedJuceInitialiser_GUI initialiser;
        for(double rate:{44100.,48000.,96000.}) for(int channels:{1,2}) {
            auto reference=render(rate,channels,{64});
            compare(reference,render(rate,channels,{32}));
            compare(reference,render(rate,channels,{0,1,127,256,512,2048}));
            XiffectsAudioProcessor source;
            for(const auto* id:{"POWER","OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON"}) set(source,id,1);
            set(source,"TUNE_STRENGTH",100);set(source,"TUNE_MIX",100);
            juce::MemoryBlock state;source.getStateInformation(state);
            compare(reference,render(rate,channels,{64},&state));
        }
        std::cout<<"Actual Reference processor: active-chain block invariance and state-render roundtrip passed\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
