#pragma once
#include <JuceHeader.h>
#include "DSP/Effects.h"

class XiffectsAudioProcessor final : public juce::AudioProcessor
{
public:
    XiffectsAudioProcessor();
    ~XiffectsAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Xiffects"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 300.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::atomic<float> inputPeak {0}, outputPeak {0};

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

private:
    enum Control { compOn, revOn, delayOn, distOn, eqOn, phaserOn,
        threshold, ratio, makeup, revMix, revSize, decay, delayMix, delayTime,
        feedback, drive, tone, distMix, low, mid, high, air, rate, depth,
        phaserFeedback, output, power, controlCount };
    std::array<std::atomic<float>*, controlCount> values{};
    std::array<xiffects::Smoother, controlCount> smooth;
    xiffects::Compressor compressor;
    xiffects::Delay delay;
    xiffects::Distortion distortion;
    xiffects::Equalizer equalizer;
    xiffects::Phaser phaser;
    xiffects::Reverb reverb;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XiffectsAudioProcessor)
};
