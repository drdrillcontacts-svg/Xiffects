#pragma once
// Actual processor tests: no duplicate signal path. Signals are synthetic, not sax recordings.
inline void testInsertQuality() {
    if(!XiffectsAudioProcessor::isLiveMode())return;
    auto check=[](bool b,const char* text){if(!b)throw std::runtime_error(text);};
    for(double sr:{44100.,48000.,96000.})for(int channels:{1,2})for(bool compressed:{false,true}) {
        XiffectsAudioProcessor p;
        auto layout=p.getBusesLayout();auto bus=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
        layout.inputBuses.set(0,bus);layout.outputBuses.set(0,bus);check(p.setBusesLayout(layout),"Insert layout");
        auto set=[&](const char* id,float v){auto* a=p.parameters.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));};
        // Fresh defaults, without disabling EQ in the test itself.
        check(p.parameters.getRawParameterValue("EQ_ON")->load()==0,"Fresh Live EQ off");
        if(compressed){set("COMP_ON",1);set("COMP_MAKEUP",0);}
        p.prepareToPlay(sr,257);const int latency=p.getLatencySamples();
        const int window=int(sr*.016)+(int(sr*.016)%2);
        check(latency==window*3/4+2,"Unchanged Live nominal delay");
        xiffects::Compressor reference;reference.prepare(sr);
        auto input=[&](int n,int c){if(n<0)return 0.f;const double t=n/sr;return float((n%4096<900?.55:.025)*(std::sin(2*xiffects::pi*452*t+c*.6)+.2*std::sin(2*xiffects::pi*7232*t))*.7);};
        int position=0;double maximum=0;
        for(int iteration=0;iteration<70;++iteration) {
            const int sizes[]={0,1,17,64,257,1024,127};int size=sizes[iteration%7];
            juce::AudioBuffer<float> audio(channels,size);juce::MidiBuffer midi;
            for(int n=0;n<size;++n)for(int c=0;c<channels;++c)audio.setSample(c,n,input(position+n,c));
            p.processBlock(audio,midi);
            for(int n=0;n<size;++n) {
                float peak=0;for(int c=0;c<channels;++c)peak=std::max(peak,std::abs(input(position+n-latency,c)));
                const float gain=compressed?reference.gain(peak,-18,4,0):1;
                for(int c=0;c<channels;++c)maximum=std::max(maximum,double(std::abs(audio.getSample(c,n)-input(position+n-latency,c)*gain)));
            }
            position+=size;
        }
        check(maximum<2e-6,"Fresh insert null / delayed sidechain regression");
        std::cout<<"INSERT_QUALITY sr="<<sr<<" channels="<<channels<<" compressor="<<compressed<<" delay="<<latency<<" max_error="<<maximum<<'\n';
        p.loadFactoryPreset(12);p.loadCleanState();
        for(auto* id:{"OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON"})check(p.parameters.getRawParameterValue(id)->load()==0,"Clean disables all effects");
        // Normalised JUCE float round-trips can leave micro-dB rounding residue.
        check(std::abs(p.parameters.getRawParameterValue("OUTPUT")->load())<1e-4f,"Clean unity output within 0.0001 dB");
    }
}
