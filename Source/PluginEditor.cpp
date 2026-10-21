#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff101114), panel (0xff17181c), red (0xffff294b), soft (0xffb9b9c0), white (0xfff4f4f6);
    void styleSlider (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        s.setColour (juce::Slider::rotarySliderFillColourId, red);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff383940));
        s.setColour (juce::Slider::thumbColourId, white);
        s.setColour (juce::Slider::textBoxTextColourId, soft);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }
    void styleCombo (juce::ComboBox& c)
    {
        c.setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff111216));
        c.setColour (juce::ComboBox::textColourId, white);
        c.setColour (juce::ComboBox::outlineColourId, juce::Colour(0xff3a3b42));
        c.setColour (juce::ComboBox::arrowColourId, red);
    }
}

XiffectsAudioProcessorEditor::XiffectsAudioProcessorEditor (XiffectsAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1440, 900);
    brand.setText ("Xiffects", juce::dontSendNotification);
    brand.setFont (juce::Font (42.0f, juce::Font::plain));
    brand.setColour (juce::Label::textColourId, white);
    brand.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (brand);
    byline.setText ("UNIQUE SOUNDS BY XIVAN", juce::dontSendNotification);
    byline.setFont (juce::Font (11.0f));
    byline.setColour (juce::Label::textColourId, soft);
    addAndMakeVisible (byline);
    saxLabel.setText ("SOPRANO SAXOPHONE  /  MULTI-EFFECT", juce::dontSendNotification);
    saxLabel.setFont (juce::Font (11.0f));
    saxLabel.setColour (juce::Label::textColourId, soft);
    addAndMakeVisible (saxLabel);

    presetBox.addItem ("Default Preset", 1);
    presetBox.addItem ("Soprano Warmth", 2);
    presetBox.addItem ("Red Lead", 3);
    presetBox.addItem ("Wide & Airy", 4);
    styleCombo (presetBox);
    addAndMakeVisible (presetBox);

    const std::array<juce::String, 9> titles = { "OCTAVER", "COMPRESSOR", "REVERB", "DELAY", "DISTORTION", "EQ", "PITCH SHIFTER", "PHASER", "AUTO TUNE" };
    const std::array<juce::StringArray, 9> ids = {{
        {"OCT_MIX","OCT_SHIFT"}, {"COMP_THRESHOLD","COMP_RATIO","COMP_MAKEUP"},
        {"REV_MIX","REV_SIZE","REV_DECAY"}, {"DLY_MIX","DLY_TIME","DLY_FEEDBACK"},
        {"DIST_DRIVE","DIST_TONE","DIST_MIX"}, {"EQ_LOW","EQ_MID","EQ_HIGH","EQ_AIR"},
        {"PITCH_SEMITONES","PITCH_MIX"}, {"PHASER_RATE","PHASER_DEPTH","PHASER_FEEDBACK"},
        {"TUNE_STRENGTH","TUNE_SPEED","TUNE_MIX"}
    }};
    const std::array<juce::StringArray, 9> labels = {{
        {"MIX","SHIFT"}, {"THRESHOLD","RATIO","MAKEUP"}, {"MIX","SIZE","DECAY"},
        {"MIX","TIME","FEEDBACK"}, {"DRIVE","TONE","MIX"}, {"LOW","MID","HIGH","AIR"},
        {"PITCH","MIX"}, {"RATE","DEPTH","FEEDBACK"}, {"STRENGTH","RETUNE","MIX"}
    }};
    const std::array<juce::String, 9> enabledIds = { "OCT_ON","COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PITCH_ON","PHASER_ON","TUNE_ON" };

    for (int i = 0; i < 9; ++i)
    {
        auto m = std::make_unique<Module>();
        m->title = titles[(size_t)i];
        m->titleLabel.setText (m->title, juce::dontSendNotification);
        m->titleLabel.setFont (juce::Font (10.5f, juce::Font::bold));
        m->titleLabel.setColour (juce::Label::textColourId, white);
        m->titleLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (m->titleLabel);
        m->parameterIds = ids[(size_t)i];
        m->labels = labels[(size_t)i];
        m->bypass.setButtonText ("●");
        m->bypass.setClickingTogglesState (true);
        m->bypass.setColour (juce::TextButton::buttonColourId, panel);
        m->bypass.setColour (juce::TextButton::buttonOnColourId, red);
        m->bypass.setColour (juce::TextButton::textColourOffId, red);
        m->bypass.setColour (juce::TextButton::textColourOnId, white);
        addAndMakeVisible (m->bypass);
        auto* raw = m.get();
        m->bypass.onClick = [this, raw, id = enabledIds[(size_t)i]] {
            if (auto* v = processor.parameters.getParameter (id))
                v->setValueNotifyingHost (raw->bypass.getToggleState() ? 1.0f : 0.0f);
        };
        for (int j = 0; j < m->parameterIds.size(); ++j)
        {
            auto s = std::make_unique<juce::Slider>();
            styleSlider (*s);
            auto c = std::make_unique<juce::Label>();
            c->setText (m->labels[j], juce::dontSendNotification);
            c->setFont (juce::Font (10.0f));
            c->setColour (juce::Label::textColourId, soft);
            c->setJustificationType (juce::Justification::centred);
            addAndMakeVisible (*s);
            addAndMakeVisible (*c);
            m->attachments.push_back (std::make_unique<Attachment> (processor.parameters, m->parameterIds[j], *s));
            m->sliders.push_back (std::move(s));
            m->captions.push_back (std::move(c));
        }
        modules.push_back (std::move(m));
    }

    tuneTitle.setText ("AUTO TUNE  /  NOTE MAP", juce::dontSendNotification);
    tuneTitle.setFont (juce::Font (15.0f, juce::Font::bold));
    tuneTitle.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (tuneTitle);
    keyLabel.setText ("KEY", juce::dontSendNotification);
    scaleLabel.setText ("SCALE", juce::dontSendNotification);
    strengthLabel.setText ("CORRECTION STRENGTH", juce::dontSendNotification);
    speedLabel.setText ("RETUNE SPEED", juce::dontSendNotification);
    for (auto* l : { &keyLabel, &scaleLabel, &strengthLabel, &speedLabel })
    {
        l->setFont (juce::Font (10.0f));
        l->setColour (juce::Label::textColourId, soft);
        addAndMakeVisible (*l);
    }
    keyBox.addItemList (juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    scaleBox.addItemList (juce::StringArray{"Chromatic","Major","Minor","Harmonic Minor","Melodic Minor","Dorian","Mixolydian","Pentatonic"}, 1);
    styleCombo (keyBox); styleCombo (scaleBox);
    addAndMakeVisible (keyBox); addAndMakeVisible (scaleBox);
    keyAttachment = std::make_unique<ComboAttachment> (processor.parameters, "TUNE_KEY", keyBox);
    scaleAttachment = std::make_unique<ComboAttachment> (processor.parameters, "TUNE_SCALE", scaleBox);

    for (int i = 0; i < 12; ++i)
    {
        const juce::String noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        noteButtons[(size_t)i].setButtonText (noteNames[i]);
        noteButtons[(size_t)i].setClickingTogglesState (true);
        noteButtons[(size_t)i].setToggleState (true, juce::dontSendNotification);
        noteButtons[(size_t)i].setColour (juce::ToggleButton::textColourId, white);
        noteButtons[(size_t)i].setColour (juce::ToggleButton::tickColourId, red);
        addAndMakeVisible (noteButtons[(size_t)i]);
        noteAttachments[(size_t)i] = std::make_unique<XiffectsAudioProcessor::APVTS::ButtonAttachment>(
            processor.parameters, "TUNE_NOTE_" + juce::String(i), noteButtons[(size_t)i]);
    }

    styleSlider (tuneStrength); styleSlider (tuneSpeed);
    addAndMakeVisible (tuneStrength); addAndMakeVisible (tuneSpeed);
    strengthAttachment = std::make_unique<Attachment> (processor.parameters, "TUNE_STRENGTH", tuneStrength);
    speedAttachment = std::make_unique<Attachment> (processor.parameters, "TUNE_SPEED", tuneSpeed);

    powerButton.setColour (juce::TextButton::buttonColourId, panel);
    powerButton.setColour (juce::TextButton::textColourOffId, red);
    addAndMakeVisible (powerButton);
}

void XiffectsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff202126));
    g.drawRoundedRectangle (bounds.reduced (12.0f), 15.0f, 1.0f);
    g.setColour (juce::Colour (0xff191a1f));
    g.fillRoundedRectangle (juce::Rectangle<float>(22, 105, getWidth()-44, 300), 12.0f);
    g.setColour (juce::Colour (0xff260b12));
    g.fillEllipse (getWidth()*0.39f, 118.0f, 310.0f, 270.0f);
    g.setColour (red.withAlpha (0.25f));
    g.drawEllipse (getWidth()*0.39f, 118.0f, 310.0f, 270.0f, 2.0f);
    g.setColour (juce::Colour (0xff25262c));
    g.drawLine (26.0f, 96.0f, getWidth()-26.0f, 96.0f, 1.0f);

    // Custom abstract sax silhouette / sound-wave hero art. Replace with licensed portrait artwork later.
    g.setColour (juce::Colour (0xff07080a).withAlpha (0.85f));
    juce::Path silhouette;
    silhouette.startNewSubPath (getWidth()*0.48f, 155.0f);
    silhouette.cubicTo (getWidth()*0.43f, 165.0f, getWidth()*0.44f, 205.0f, getWidth()*0.42f, 240.0f);
    silhouette.cubicTo (getWidth()*0.40f, 265.0f, getWidth()*0.43f, 290.0f, getWidth()*0.45f, 315.0f);
    silhouette.lineTo (getWidth()*0.57f, 315.0f);
    silhouette.cubicTo (getWidth()*0.59f, 270.0f, getWidth()*0.58f, 220.0f, getWidth()*0.54f, 175.0f);
    silhouette.closeSubPath();
    g.fillPath (silhouette);
    g.setColour (red.withAlpha (0.7f));
    for (int i = 0; i < 5; ++i)
    {
        juce::Path wave;
        wave.startNewSubPath (40.0f, 300.0f + i*8.0f);
        wave.cubicTo (240.0f, 245.0f-i*8.0f, 340.0f, 365.0f+i*3.0f, 520.0f, 300.0f);
        wave.cubicTo (760.0f, 235.0f+i*4.0f, 890.0f, 365.0f-i*5.0f, 1110.0f, 295.0f+i*7.0f);
        g.strokePath (wave, juce::PathStrokeType (1.0f));
    }
    g.setColour (red);
    g.setFont (juce::Font (46.0f, juce::Font::italic));
    g.drawText ("Xiffects", 70, 170, 420, 75, juce::Justification::left);
    g.setColour (soft);
    g.setFont (juce::Font (10.0f));
    g.drawText ("SHAPE THE SOUND OF YOUR SAX", 76, 245, 350, 22, juce::Justification::left);
    g.setColour (panel);
    g.fillRoundedRectangle (juce::Rectangle<float>(22, 420, getWidth()-44, 380), 12.0f);
    g.setColour (juce::Colour (0xff303137));
    g.drawRoundedRectangle (juce::Rectangle<float>(22, 420, getWidth()-44, 380), 12.0f, 1.0f);
    g.setColour (juce::Colour (0xff202126));
    g.fillRoundedRectangle (juce::Rectangle<float>(22, 812, getWidth()-44, 70), 10.0f);
    g.setColour (red.withAlpha (0.8f));
    g.drawLine (35, 812, getWidth()-35, 812, 1.0f);
    g.setColour (soft);
    g.setFont (juce::Font (10.0f));
    g.drawText ("SOPRANO SAX", 48, 838, 150, 18, juce::Justification::left);
    g.drawText ("INPUT  -6.0 dB", 240, 838, 150, 18, juce::Justification::left);
    g.drawText ("OUTPUT  -6.0 dB", getWidth()-220, 838, 160, 18, juce::Justification::right);
    g.setColour (red);
    for (int i = 0; i < 44; ++i)
    {
        float h = 4.0f + (float)((i * 17 + 13) % 23);
        g.fillRoundedRectangle (400.0f + i*9.0f, 847.0f-h/2.0f, 3.0f, h, 1.5f);
    }
}

void XiffectsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    brand.setBounds (60, 22, 260, 52);
    byline.setBounds (330, 36, 230, 24);
    saxLabel.setBounds (570, 36, 280, 24);
    presetBox.setBounds (area.getWidth()-365, 28, 210, 34);
    powerButton.setBounds (area.getWidth()-105, 28, 42, 34);

    const int moduleY = 430, moduleH = 365;
    const int left = 28, totalW = getWidth()-56, moduleW = totalW/9;
    for (int i = 0; i < (int)modules.size(); ++i)
    {
        auto& m = *modules[(size_t)i];
        const int x = left + i*moduleW;
        m.titleLabel.setBounds (x+4, moduleY+30, moduleW-38, 28);
        m.bypass.setBounds (x+moduleW-31, moduleY+8, 22, 22);
        for (int j = 0; j < (int)m.sliders.size(); ++j)
        {
            const int count = (int)m.sliders.size();
            const int cellW = (moduleW-12) / count;
            const int sx = x + 6 + j*cellW;
            m.captions[(size_t)j]->setBounds (sx, moduleY+70, cellW, 20);
            m.sliders[(size_t)j]->setBounds (sx, moduleY+95, cellW, count == 4 ? 125 : 140);
        }
        // Title is painted in the component background area.
    }

    tuneTitle.setBounds (45, 816, 210, 25);
    keyLabel.setBounds (55, 845, 45, 16);
    keyBox.setBounds (55, 862, 90, 24);
    scaleLabel.setBounds (158, 845, 55, 16);
    scaleBox.setBounds (158, 862, 125, 24);
    const int noteX = 310, noteY = 852, noteW = 42;
    for (int i = 0; i < 12; ++i)
        noteButtons[(size_t)i].setBounds (noteX+i*noteW, noteY, noteW, 28);
    strengthLabel.setBounds (850, 832, 145, 18);
    tuneStrength.setBounds (980, 822, 110, 55);
    speedLabel.setBounds (1110, 832, 100, 18);
    tuneSpeed.setBounds (1200, 822, 110, 55);
}
