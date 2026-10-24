#pragma once
inline void testProductPitchQuality() {
    for(double sr:{44100.,48000.,96000.})for(float hz:{452.f,904.f})for(float strength:{50.f,100.f}) {
        XiffectsAudioProcessor p;
        auto set=[&](const char* id,float v){auto* a=p.parameters.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));};
        for(auto* id:{"OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON"})set(id,0);
        set("TUNE_ON",1);set("TUNE_MIX",100);set("TUNE_STRENGTH",strength);set("TUNE_SPEED",12);set("TUNE_SCALE",0);set("TUNE_SCALE_EXT",0);set("OUTPUT",0);
        p.prepareToPlay(sr,64);juce::MidiBuffer midi;int crossings=0;double first=0,last=0;float previous=0;
        for(int position=0;position<int(sr);position+=64) {
            int size=std::min(64,int(sr)-position);juce::AudioBuffer<float> audio(2,size);
            for(int n=0;n<size;++n){double t=(position+n)/sr;float in=t<.1?0:float(.2*std::sin(2*double(xiffects::pi)*hz*t)*std::min(1.,(t-.1)/.005));audio.setSample(0,n,in);audio.setSample(1,n,in);}
            p.processBlock(audio,midi);
            for(int n=0;n<size;++n){float v=audio.getSample(0,n);if((position+n)/sr>.6 && previous<=0 && v>0){double at=position+n-v/(v-previous);if(crossings==0)first=at;last=at;++crossings;}previous=v;}
        }
        double target=hz==452?440:880, expected=hz*std::pow(target/hz,strength/100), measured=(crossings-1)*sr/(last-first);
        double error=1200*std::log2(measured/expected);
        if(crossings<2 || !std::isfinite(error) || std::abs(error)>5)throw std::runtime_error("Actual processor pitch strength / boundary regression");
        std::cout<<"PRODUCT_PITCH "<<p.getName()<<" sr="<<sr<<" input="<<hz<<" strength="<<strength<<" hz="<<measured<<" cents="<<error<<'\n';
    }
}
inline void testEditorNavigation() {
    XiffectsAudioProcessor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    auto click=[&](const char* text){for(int i=0;i<editor->getNumChildComponents();++i)if(auto* b=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i)))if(b->getButtonText()==text){b->onClick();return;}throw std::runtime_error("Missing preset navigation");};
    auto verify=[&](int index){auto* values=p.factoryPresets()[index]["parameters"].getDynamicObject();for(auto* param:p.getParameters()){auto* a=dynamic_cast<juce::RangedAudioParameter*>(param);if(std::abs(a->getValue()-a->convertTo0to1(float(values->getProperty(a->paramID))))>1e-5f)throw std::runtime_error("Preset navigation did not restore full recipe");}};
    click(">");verify(0);click("<");verify(49);click(">");verify(0);click(">");verify(1);
    editor->setSize(1152,720);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile(p.isLiveMode()?"Xiffects-Live-compact.png":"Xiffects-Studio-compact.png");
    auto stream=file.createOutputStream();if(!stream || !juce::PNGImageFormat().writeImageToStream(image,*stream))throw std::runtime_error("UI snapshot");
    std::cout<<"Preset previous/next/wrap and compact native UI snapshot passed\n";
}
