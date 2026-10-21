#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void set(XiffectsAudioProcessor& p,const char* id,float v){auto* a=p.parameters.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));}
int main(){try {
    juce::ScopedJuceInitialiser_GUI gui;
    check(XiffectsAudioProcessor::isLiveMode(),"Must link actual Live product");
    for(double sr:{44100.,48000.,96000.}) for(int channels:{1,2}) for(bool bypass:{false,true}) {
        XiffectsAudioProcessor p;
        auto layout=p.getBusesLayout();const auto channelSet=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
        layout.inputBuses.set(0,channelSet);layout.outputBuses.set(0,channelSet);check(p.setBusesLayout(layout),"Layout");
        for(auto* id:{"OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON"})set(p,id,0);
        set(p,"POWER",bypass?0.f:1.f);p.prepareToPlay(sr,64);
        const int latency=p.getLatencySamples();check(latency>0 && latency<sr*.013,"Live host latency under13ms");
        int position=0,found=-1;
        for(int size:{0,32,64,128,256,512,2048}) {
            juce::AudioBuffer<float> audio(channels,size);audio.clear();if(position==0 && size>0)audio.setSample(0,0,.5f);
            juce::MidiBuffer midi;p.processBlock(audio,midi);
            for(int n=0;n<size;++n) for(int c=0;c<channels;++c) {
                float expected=c==0 && position+n==latency?.5f:0;
                check(std::abs(audio.getSample(c,n)-expected)<1.e-6f,"Actual product impulse must match reported delay");
                if(c==0 && audio.getSample(c,n)>.25f)found=position+n;
            }
            position+=size;
        }
        check(found==latency,"Measured product latency");
        std::cout<<"LIVE rate="<<sr<<" channels="<<channels<<" bypass="<<bypass<<" measured_samples="<<found<<" ms="<<1000*found/sr<<'\n';
        set(p,"TUNE_ON",1);set(p,"TUNE_KEY",7);set(p,"TUNE_NOTE_9",0);
        juce::MemoryBlock state;p.getStateInformation(state);XiffectsAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));
        for(auto* parameter:p.getParameters()) {
            auto* r=dynamic_cast<juce::RangedAudioParameter*>(parameter);
            check(std::abs(r->getValue()-restored.parameters.getParameter(r->paramID)->getValue())<1e-6f,"Live state roundtrip");
        }
    }
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
