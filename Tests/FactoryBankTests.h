#pragma once
#include <set>
#include <stdexcept>
#include "TimeHostTests.h"
#include "ProductQualityTests.h"
inline void testFactoryBank() {
    auto check=[](bool value,const char* text){if(!value)throw std::runtime_error(text);};
    testHostTempo();testProductPitchQuality();testEditorNavigation();
    XiffectsAudioProcessor legacy;
    auto old=legacy.parameters.copyState();
    old.removeChild(old.getChildWithProperty("id","TUNE_SCALE_EXT"),nullptr);
    const auto migrated=XiffectsAudioProcessor::migrateState(old);
    check(migrated.getChildWithProperty("id","TUNE_SCALE_EXT").isValid(),"Legacy extended selector migration");
    check(float(migrated.getChildWithProperty("id","TUNE_SCALE_EXT")["value"])==0,"Legacy scale remains effective");
    const auto& bank=XiffectsAudioProcessor::factoryPresets();check(bank.size()==50,"50 factory snapshots");
    std::set<std::string> names,snapshots;
    for(int i=0;i<bank.size();++i) {
        check(names.insert(bank[i]["name"].toString().toStdString()).second,"Unique preset name");
        check(snapshots.insert(juce::JSON::toString(bank[i]["parameters"]).toStdString()).second,"Unique complete sound recipe");
        for(double sr:{44100.,48000.,96000.}) {
            XiffectsAudioProcessor p;p.loadFactoryPreset(i);
            auto* object=bank[i]["parameters"].getDynamicObject();
            check(object->getProperties().size()==p.getParameters().size(),"Every parameter included");
            for(auto* parameter:p.getParameters()) {
                auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);
                check(object->hasProperty(ranged->paramID),"Preset includes parameter");
                float desired=float(object->getProperty(ranged->paramID));
                check(desired>=ranged->getNormalisableRange().start && desired<=ranged->getNormalisableRange().end,"Preset within range");
                check(std::abs(ranged->getValue()-ranged->convertTo0to1(desired))<1e-5f,"Applied recipe");
            }
            juce::MemoryBlock saved;p.getStateInformation(saved);
            XiffectsAudioProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
            for(auto* parameter:p.getParameters()) {
                auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);
                check(std::abs(ranged->getValue()-restored.parameters.getParameter(ranged->paramID)->getValue())<1e-6f,"Complete preset restoration");
            }
            p.prepareToPlay(sr,64);juce::MidiBuffer midi;
            for(int block=0;block<100;++block) {
                juce::AudioBuffer<float> audio(2,64);
                for(int c=0;c<2;++c)for(int n=0;n<64;++n)audio.setSample(c,n,.1f*std::sin(float(2*xiffects::pi*452*(block*64+n)/sr)));
                p.processBlock(audio,midi);
                for(int c=0;c<2;++c)for(int n=0;n<64;++n)check(std::isfinite(audio.getSample(c,n)) && std::abs(audio.getSample(c,n))<=1,"Finite bounded factory audio");
            }
        }
    }
    std::cout<<"50 complete presets: ranges, uniqueness, actual audio and state restoration at three rates passed\n";
}
