#pragma once
struct TestTempoPlayhead final : juce::AudioPlayHead {
    double bpm=120;bool available=true;
    juce::Optional<PositionInfo> getPosition() const override {if(!available)return {};PositionInfo p;p.setBpm(bpm);return p;}
};
inline void testHostTempo() {
    TestTempoPlayhead host;XiffectsAudioProcessor p;p.setPlayHead(&host);
    auto set=[&](const char* id,float value){auto* a=p.parameters.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(value));};
    for(const char* id:{"OCT_ON","COMP_ON","REV_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON"})set(id,0);
    set("DLY_ON",1);set("DLY_MIX",100);set("DLY_FEEDBACK",0);set("DLY_SYNC",1);set("DLY_DIVISION",2);set("OUTPUT",0);
    p.prepareToPlay(48000,64);juce::MidiBuffer midi;
    auto process=[&]{juce::AudioBuffer<float> b(2,64);b.clear();p.processBlock(b,midi);};
    process();if(p.tempoBpm.load()!=120 || !p.tempoFromHost.load())throw std::runtime_error("Host BPM");
    host.available=false;process();if(p.tempoFromHost.load() || p.tempoBpm.load()!=120)throw std::runtime_error("Fallback BPM");
    host.available=true;host.bpm=60;process();if(p.tempoBpm.load()!=60)throw std::runtime_error("Tempo automation");
    host.bpm=120;for(int i=0;i<2250;++i)process();
    int peak=-1;float maximum=0;
    for(int i=0;i<400;++i){juce::AudioBuffer<float> b(2,64);b.clear();if(i==0)b.setSample(0,0,.5f);p.processBlock(b,midi);
        for(int j=0;j<64;++j)if(b.getSample(0,j)>maximum){maximum=b.getSample(0,j);peak=i*64+j;}}
    if(maximum<.1f || std::abs(peak-(12000+p.getLatencySamples()))>10)throw std::runtime_error("Actual synced delay audio");
    std::cout<<"Actual processor host BPM, missing-tempo fallback and synced impulse passed\n";
}
