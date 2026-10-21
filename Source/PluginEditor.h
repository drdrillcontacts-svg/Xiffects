#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class XiffectsAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit XiffectsAudioProcessorEditor (XiffectsAudioProcessor&);
    ~XiffectsAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    juce::Image hero;
    juce::Slider outputSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    XiffectsAudioProcessor& processor;

    struct Module
    {
        juce::String title;
        juce::Label titleLabel;
        juce::StringArray parameterIds;
        juce::StringArray labels;
        juce::TextButton bypass;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        std::vector<std::unique_ptr<juce::Slider>> sliders;
        std::vector<std::unique_ptr<juce::Label>> captions;
        std::vector<std::unique_ptr<Attachment>> attachments;
    };
    std::vector<std::unique_ptr<Module>> modules;
    juce::ComboBox presetBox, keyBox, scaleBox;
    std::unique_ptr<ComboAttachment> keyAttachment, scaleAttachment;
    juce::Label brand, byline, saxLabel, tuneTitle, keyLabel, scaleLabel, strengthLabel, speedLabel;
    juce::Slider tuneStrength, tuneSpeed;
    std::unique_ptr<Attachment> strengthAttachment, speedAttachment;
    std::array<juce::ToggleButton, 12> noteButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> noteAttachments;
    juce::TextButton powerButton { "⏻" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XiffectsAudioProcessorEditor)
};
