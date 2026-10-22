#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/Controls.h"

class XiffectsAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit XiffectsAudioProcessorEditor (XiffectsAudioProcessor&);
    ~XiffectsAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
#if JUCE_IOS || JUCE_ANDROID
    juce::ComboBox touchPage, touchModule;
    void layoutTouch();
    void paintTouch(juce::Graphics&);
#endif
    void choosePresetFile(bool save);
    void showSettings();
    xiffects::LookAndFeel lookAndFeel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::TooltipWindow tooltips {this,700};
    juce::TextButton saveButton {"SAVE"}, loadButton {"LOAD"}, settingsButton {"..."};
    std::array<xiffects::TelemetryFrame,240> history{};
    size_t historyPosition=0;
    float displayInput=0,displayOutput=0;
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
    std::array<xiffects::NoteKey, 12> noteButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> noteAttachments;
    juce::TextButton powerButton { "⏻" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XiffectsAudioProcessorEditor)
};
