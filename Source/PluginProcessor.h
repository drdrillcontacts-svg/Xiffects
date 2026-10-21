#pragma once
#include <JuceHeader.h>
#include "DSP/Effects.h"
#include "DSP/Pitch.h"
#include "DSP/LivePitchBank.h"
#ifndef XIFFECTS_LIVE_MODE
#define XIFFECTS_LIVE_MODE 0
#endif
#include "Telemetry.h"

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

    static constexpr bool isLiveMode() { return XIFFECTS_LIVE_MODE != 0; }
    const juce::String getName() const override { return isLiveMode()?"Xiffects Live":"Xiffects"; }
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
    xiffects::TelemetryQueue telemetry;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

private:
    enum Control { compOn, revOn, delayOn, distOn, eqOn, phaserOn,
        threshold, ratio, makeup, revMix, revSize, decay, delayMix, delayTime,
        feedback, drive, tone, distMix, low, mid, high, air, rate, depth,
        phaserFeedback, output, power, octOn, octMix, octShift, pitchOn, pitchMix, pitchShift,
        tuneOn, tuneStrength, tuneSpeed, tuneMix, tuneKey, tuneScale, controlCount };
    std::array<std::atomic<float>*, controlCount> values{};
    std::array<xiffects::Smoother, controlCount> smooth;
    std::array<std::atomic<float>*,12> noteValues{};
    xiffects::PitchCorrection correction;
    xiffects::LivePitchBank livePitch;
    xiffects::PitchShifter tuneShifter, octaveShifter, pitchShifter;
    std::array<std::vector<float>,2> bypassDelay;
    size_t bypassPosition=0;
    int telemetryCounter=0;
    float waveformMin=0, waveformMax=0;
    xiffects::Compressor compressor;
    xiffects::Delay delay;
    xiffects::Distortion distortion;
    xiffects::Equalizer equalizer;
    xiffects::Phaser phaser;
    xiffects::Reverb reverb;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XiffectsAudioProcessor)
};
