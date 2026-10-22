#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

XiffectsAudioProcessor::XiffectsAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "XIFfectsState", createParameterLayout())
{
    extendedScale=parameters.getRawParameterValue("TUNE_SCALE_EXT");
    const char* ids[] = {"COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PHASER_ON",
        "COMP_THRESHOLD","COMP_RATIO","COMP_MAKEUP","REV_MIX","REV_SIZE","REV_DECAY",
        "DLY_MIX","DLY_TIME","DLY_FEEDBACK","DIST_DRIVE","DIST_TONE","DIST_MIX",
        "EQ_LOW","EQ_MID","EQ_HIGH","EQ_AIR","PHASER_RATE","PHASER_DEPTH",
        "PHASER_FEEDBACK","OUTPUT","POWER","OCT_ON","OCT_MIX","OCT_SHIFT",
        "PITCH_ON","PITCH_MIX","PITCH_SEMITONES","TUNE_ON","TUNE_STRENGTH",
        "TUNE_SPEED","TUNE_MIX","TUNE_KEY","TUNE_SCALE"};
    for (size_t i=0; i<values.size(); ++i) values[i]=parameters.getRawParameterValue(ids[i]);
    for(size_t i=0;i<noteValues.size();++i) noteValues[i]=parameters.getRawParameterValue("TUNE_NOTE_"+juce::String(int(i)));
}

XiffectsAudioProcessor::APVTS::ParameterLayout XiffectsAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    auto addFloat = [&layout](const char* id, const char* name, float lo, float hi, float def, float step = 0.1f)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (lo, hi, step), def));
    };
    auto addBool = [&layout](const char* id, const char* name, bool def = true)
    {
        layout.add (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { id, 1 }, name, def));
    };

    const char* modules[] = { "OCT", "COMP", "REV", "DLY", "DIST", "EQ", "PITCH", "PHASER", "TUNE" };
    const char* names[]   = { "Octaver", "Compressor", "Reverb", "Delay", "Distortion", "EQ", "Pitch Shifter", "Phaser", "Auto Tune" };
    for (int i = 0; i < 9; ++i)
    {
        auto id = juce::String (modules[i]) + "_ON";
        auto nm = juce::String (names[i]) + " Enabled";
        addBool (id.toRawUTF8(), nm.toRawUTF8(), i == 5 ? true : false);
    }

    addFloat("OCT_MIX", "Octaver Mix", 0, 100, 35);
    addFloat("OCT_SHIFT", "Octaver Shift", -2, 2, -1, 1);
    addFloat("COMP_THRESHOLD", "Compressor Threshold", -60, 0, -18);
    addFloat("COMP_RATIO", "Compressor Ratio", 1, 20, 4, 0.1f);
    addFloat("COMP_MAKEUP", "Compressor Makeup", 0, 18, 3);
    addFloat("REV_MIX", "Reverb Mix", 0, 100, 28);
    addFloat("REV_SIZE", "Reverb Size", 0, 100, 45);
    addFloat("REV_DECAY", "Reverb Decay", 0.1f, 10, 2.8f);
    addFloat("DLY_MIX", "Delay Mix", 0, 100, 32);
    addFloat("DLY_TIME", "Delay Time", 0.01f, 2, 0.5f, 0.01f);
    addFloat("DLY_FEEDBACK", "Delay Feedback", 0, 95, 38);
    addFloat("DIST_DRIVE", "Distortion Drive", 0, 100, 22);
    addFloat("DIST_TONE", "Distortion Tone", 0, 100, 50);
    addFloat("DIST_MIX", "Distortion Mix", 0, 100, 32);
    addFloat("EQ_LOW", "EQ Low", -12, 12, -2);
    addFloat("EQ_MID", "EQ Mid", -12, 12, 1.5f);
    addFloat("EQ_HIGH", "EQ High", -12, 12, 2);
    addFloat("EQ_AIR", "EQ Air", -12, 12, 3);
    addFloat("PITCH_SEMITONES", "Pitch Shift", -12, 12, 7, 1);
    addFloat("PITCH_MIX", "Pitch Mix", 0, 100, 32);
    addFloat("PHASER_RATE", "Phaser Rate", 0.05f, 8, 0.6f);
    addFloat("PHASER_DEPTH", "Phaser Depth", 0, 100, 46);
    addFloat("PHASER_FEEDBACK", "Phaser Feedback", 0, 100, 35);
    addFloat("TUNE_STRENGTH", "Correction Strength", 0, 100, 26);
    addFloat("TUNE_SPEED", "Retune Speed", 1, 100, 12);
    addFloat("TUNE_MIX", "Auto Tune Mix", 0, 100, 70);
    addBool("POWER", "Processing Enabled", true);
    addFloat("OUTPUT", "Output", -24, 12, 0);
    // Auto Tune key/scale and manually allowed notes are stored as parameters for host automation.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TUNE_KEY", 1}, "Auto Tune Key",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TUNE_SCALE", 1}, "Auto Tune Scale",
        juce::StringArray{"Chromatic","Major","Minor","Harmonic Minor","Melodic Minor","Dorian","Mixolydian","Pentatonic"}, 1));
    juce::StringArray extensions {"Use legacy scale"};
    for(size_t i=8;i<xiffects::scales.size();++i) extensions.add(xiffects::scales[i].name);
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"TUNE_SCALE_EXT",1},"Extended Scale",extensions,0));
    for (int n = 0; n < 12; ++n)
        addBool ((juce::String("TUNE_NOTE_") + juce::String(n)).toRawUTF8(),
                 (juce::String("Allow Note ") + juce::String(n)).toRawUTF8(), true);
    return layout;
}

void XiffectsAudioProcessor::prepareToPlay (double sampleRate, int)
{
    correction.prepare(sampleRate); tuneShifter.prepare(sampleRate);
    octaveShifter.prepare(sampleRate); pitchShifter.prepare(sampleRate);
    if(isLiveMode()) livePitch.prepare(sampleRate);
    const int latency=isLiveMode()?livePitch.latency():tuneShifter.latency()+octaveShifter.latency();
    setLatencySamples(latency);
    for(auto& channel:bypassDelay) channel.assign(size_t(latency),0);
    bypassPosition=0; telemetryCounter=0; waveformMin=waveformMax=0;
    compressor.prepare(sampleRate); delay.prepare(sampleRate);
    distortion.prepare(sampleRate); equalizer.prepare(sampleRate);
    phaser.prepare(sampleRate); reverb.prepare(sampleRate);
    for(size_t i=0;i<values.size();++i) smooth[i].prepare(sampleRate,values[i]->load());
    inputPeak.store(0); outputPeak.store(0);
}

bool XiffectsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && out == layouts.getMainInputChannelSet();
}

void XiffectsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    std::array<float, controlCount> target{}, v{};
    for(size_t i=0;i<values.size();++i) target[i]=values[i]->load();
    std::array<bool,12> notes{};
    for(size_t i=0;i<notes.size();++i) notes[i]=noteValues[i]->load()>0.5f;
    const int activeScale=selectedScale();
    float inPeak=0, outPeak=0;
    const int channels=buffer.getNumChannels();
    const auto blend=[](float dry,float wet,float mix) { return dry+(wet-dry)*mix; };
    for(int n=0;n<buffer.getNumSamples();++n) {
        for(size_t i=0;i<v.size();++i) v[i]=smooth[i].next(target[i]);
        std::array<float,2> x {buffer.getSample(0,n), channels>1?buffer.getSample(1,n):buffer.getSample(0,n)};
        std::array<float,2> dry{};
        for(size_t c=0;c<2;++c) {
            dry[c]=bypassDelay[c][bypassPosition]; bypassDelay[c][bypassPosition]=x[c];
        }
        bypassPosition=(bypassPosition+1)%bypassDelay[0].size();
        const float peak=std::max(std::abs(x[0]),std::abs(x[1])); inPeak=std::max(inPeak,peak);
        const float tuningRatio=correction.process(x[0],int(target[tuneKey]),activeScale,
            notes,v[tuneStrength]*0.01f,v[tuneSpeed]);
        const float fundamental=correction.detector.frequency;
        if(isLiveMode()) {
            x=livePitch.process(x,tuningRatio,v[tuneOn]*v[tuneMix]*0.01f,
                std::exp2(v[octShift]),std::exp2(v[pitchShift]/12),
                v[octOn]*v[octMix]*0.01f,v[pitchOn]*v[pitchMix]*0.01f,fundamental);
        } else {
        x=tuneShifter.process(x,tuningRatio,v[tuneOn]*v[tuneMix]*0.01f,fundamental);
        const float octaveAmount=v[octOn]*v[octMix]*0.01f, pitchAmount=v[pitchOn]*v[pitchMix]*0.01f;
        const float normalizer=std::max(1.f,octaveAmount+pitchAmount);
        const float shiftedFundamental=fundamental*((v[tuneOn]*v[tuneMix]>50)?tuningRatio:1.f);
        const auto octaved=octaveShifter.process(x,std::exp2(v[octShift]),octaveAmount/normalizer,shiftedFundamental);
        const auto pitched=pitchShifter.process(x,std::exp2(v[pitchShift]/12),pitchAmount/normalizer,shiftedFundamental);
        for(size_t c=0;c<2;++c) x[c]=octaved[c]+pitched[c]-octaveShifter.dryOutput[c];
        }
        const float gain=compressor.gain(peak,v[threshold],v[ratio],v[makeup]);
        for(auto& sample:x) sample=blend(sample,sample*gain,v[compOn]);
        for(size_t c=0;c<2;++c) x[c]=blend(x[c],distortion.process(x[c],c,v[drive]*0.01f,v[tone]*0.01f),v[distOn]*v[distMix]*0.01f);
        const std::array<float,4> gains {xiffects::db(v[low]),xiffects::db(v[mid]),xiffects::db(v[high]),xiffects::db(v[air])};
        for(size_t c=0;c<2;++c) x[c]=blend(x[c],equalizer.process(x[c],c,gains),v[eqOn]);
        const auto phased=phaser.process(x,v[rate],v[depth]*0.01f,v[phaserFeedback]*0.01f);
        for(size_t c=0;c<2;++c) x[c]=blend(x[c],phased[c],v[phaserOn]);
        const auto echoes=delay.process(x,v[delayTime],v[feedback]*0.01f);
        for(size_t c=0;c<2;++c) x[c]=blend(x[c],echoes[c],v[delayOn]*v[delayMix]*0.01f);
        for(size_t c=0;c<2;++c) x[c]=blend(x[c],reverb.process(x[c],c,v[revSize]*0.01f,v[decay]),v[revOn]*v[revMix]*0.01f);
        const float outputGain=xiffects::db(v[output]);
        for(int c=0;c<channels;++c) {
            // Safety ceiling only on the processed path; global bypass remains transparent.
            const float sample=blend(dry[size_t(c)],std::clamp(x[size_t(c)]*outputGain,-1.0f,1.0f),v[power]);
            buffer.setSample(c,n,sample); outPeak=std::max(outPeak,std::abs(sample));
        }
        waveformMin=std::min(waveformMin,buffer.getSample(0,n));
        waveformMax=std::max(waveformMax,buffer.getSample(0,n));
        if(++telemetryCounter>=256) {
            const float applied=correction.detected>0?correction.detected+
                (correction.corrected-correction.detected)*v[tuneOn]*v[tuneMix]*0.01f*v[power]:0;
            telemetry.push({correction.detected,applied,waveformMin,waveformMax});
            telemetryCounter=0; waveformMin=waveformMax=0;
        }
    }
    inputPeak.store(inPeak,std::memory_order_relaxed);
    outputPeak.store(outPeak,std::memory_order_relaxed);
}

juce::AudioProcessorEditor* XiffectsAudioProcessor::createEditor()
{
    return new XiffectsAudioProcessorEditor (*this);
}

void XiffectsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void XiffectsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (migrateState(juce::ValueTree::fromXml (*xml)));
}

// Required entry point for both VST3 and Standalone wrappers.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new XiffectsAudioProcessor(); }

int XiffectsAudioProcessor::selectedScale() const {
    const int extended=int(extendedScale->load());
    return extended>0?extended+7:int(values[tuneScale]->load());
}
const juce::Array<juce::var>& XiffectsAudioProcessor::factoryPresets() {
    static const juce::var bank=juce::JSON::parse(juce::String::fromUTF8(BinaryData::factory_presets_json,BinaryData::factory_presets_jsonSize));
    return *bank["presets"].getArray();
}
void XiffectsAudioProcessor::loadFactoryPreset(int index) {
    const auto& bank=factoryPresets();
    if(index<0 || index>=bank.size()) return;
    const auto* valuesObject=bank[index]["parameters"].getDynamicObject();
    for(const auto& entry:valuesObject->getProperties()) {
        if(auto* parameter=parameters.getParameter(entry.name.toString())) {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(float(entry.value)));
            parameter->endChangeGesture();
        }
    }
}

juce::ValueTree XiffectsAudioProcessor::migrateState(juce::ValueTree state) {
    if(state.isValid() && !state.getChildWithProperty("id","TUNE_SCALE_EXT").isValid()) {
        juce::ValueTree extension("PARAM");
        extension.setProperty("id","TUNE_SCALE_EXT",nullptr);
        extension.setProperty("value",0.f,nullptr);
        state.appendChild(extension,nullptr);
    }
    return state;
}
