#include "PluginProcessor.h"
#include "PluginEditor.h"

XiffectsAudioProcessor::XiffectsAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "XIFfectsState", createParameterLayout())
{
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
    addFloat("OUTPUT", "Output", -24, 12, 0);
    // Auto Tune key/scale and manually allowed notes are stored as parameters for host automation.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TUNE_KEY", 1}, "Auto Tune Key",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TUNE_SCALE", 1}, "Auto Tune Scale",
        juce::StringArray{"Chromatic","Major","Minor","Harmonic Minor","Melodic Minor","Dorian","Mixolydian","Pentatonic"}, 1));
    for (int n = 0; n < 12; ++n)
        addBool ((juce::String("TUNE_NOTE_") + juce::String(n)).toRawUTF8(),
                 (juce::String("Allow Note ") + juce::String(n)).toRawUTF8(), true);
    return layout;
}

void XiffectsAudioProcessor::prepareToPlay (double, int samplesPerBlock)
{
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
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
    // Initial scaffold: clean pass-through plus output trim.
    // Individual effect DSP blocks are intentionally TODOs; the UI/parameters are wired first.
    const auto gainDb = parameters.getRawParameterValue("OUTPUT")->load();
    const auto gain = juce::Decibels::decibelsToGain(gainDb);
    buffer.applyGain(gain);
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
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
