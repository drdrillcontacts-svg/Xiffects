#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>

void require(bool result,const char* message) {
    if(!result) {std::cerr<<message<<std::endl;std::exit(1);}
}
void set(XiffectsAudioProcessor& p,const char* id,float value) {
    auto* parameter=p.parameters.getParameter(id);
    require(parameter!=nullptr,"Parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
int main() {
    juce::ScopedJuceInitialiser_GUI initialiser;
    for(double rate:{44100.,48000.,96000.}) for(int channels:{1,2}) {
        XiffectsAudioProcessor processor;
        auto layout=processor.getBusesLayout();
        layout.inputBuses.set(0,channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());
        layout.outputBuses.set(0,layout.inputBuses[0]);
        require(processor.setBusesLayout(layout),"Supported mono/stereo layout");
        set(processor,"POWER",0);
        processor.prepareToPlay(rate,257);
        const int latency=processor.getLatencySamples();
        require(latency>0 && latency<int(rate*0.065),"Reported latency about 60 ms");
        int total=0;
        for(int block:{0,1,64,257,127,2048,4096}) {
            juce::AudioBuffer<float> buffer(channels,block);buffer.clear();
            if(total==0 && block>0) buffer.setSample(0,0,0.75f);
            juce::MidiBuffer midi;processor.processBlock(buffer,midi);
            for(int i=0;i<block;++i) for(int c=0;c<channels;++c) {
                const float expected=(c==0 && total+i==latency)?0.75f:0;
                require(std::abs(buffer.getSample(c,i)-expected)<1.e-6f,"Latency compensated global bypass / channel isolation");
            }
            total+=block;
        }
        set(processor,"TUNE_KEY",7);set(processor,"TUNE_NOTE_9",0);set(processor,"PITCH_SEMITONES",-5);
        juce::MemoryBlock state;processor.getStateInformation(state);
        XiffectsAudioProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        for(auto* parameter:processor.getParameters()) {
            auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);
            require(ranged!=nullptr,"Ranged parameter");
            require(std::abs(restored.parameters.getParameter(ranged->paramID)->getValue()-ranged->getValue())<1.e-6f,"Full host state round trip");
        }
        processor.releaseResources();
    }
    XiffectsAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    require(editor!=nullptr && editor->getWidth()==1440,"Native editor creation");
    editor->setSize(1152,720);editor->setSize(1728,1080);
    std::cout<<"Host state, mono/stereo, variable blocks, bypass latency and editor smoke tests passed\n";
}
