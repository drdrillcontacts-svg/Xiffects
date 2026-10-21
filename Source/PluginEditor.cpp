#include "PluginEditor.h"
#include <BinaryData.h>

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
    hero = juce::ImageCache::getFromMemory(BinaryData::hero_jpg, BinaryData::hero_jpgSize);
    setResizable(true, true);
    setResizeLimits(1152, 720, 1920, 1200);
    if (auto* constrainer = getConstrainer()) constrainer->setFixedAspectRatio(1.6);
    setSize (1440, 900);
    startTimerHz(24);
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
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this] {
        auto set = [this](const char* id, float value) {
            auto* parameter = processor.parameters.getParameter(id);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
            parameter->endChangeGesture();
        };
        // Factory recipes change only implemented effects, preserving manual note choices.
        for (auto id : {"COMP_ON","REV_ON","DLY_ON","DIST_ON","EQ_ON","PHASER_ON"}) set(id,0);
        const int preset = presetBox.getSelectedId();
        if (preset > 1) { set("COMP_ON",1); set("COMP_THRESHOLD",-18); set("COMP_RATIO",3); set("COMP_MAKEUP",2); }
        if (preset == 2) { set("EQ_ON",1); set("EQ_LOW",2); set("EQ_MID",0); set("EQ_HIGH",-2); set("EQ_AIR",0); }
        if (preset == 3) { set("DIST_ON",1); set("DIST_DRIVE",24); set("DIST_MIX",25); set("DLY_ON",1); set("DLY_MIX",20); }
        if (preset == 4) { set("REV_ON",1); set("REV_MIX",30); set("REV_DECAY",3); set("PHASER_ON",1); }
    };
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
        m->bypassAttachment = std::make_unique<XiffectsAudioProcessor::APVTS::ButtonAttachment>(
            processor.parameters, enabledIds[(size_t)i], m->bypass);
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
    powerButton.setButtonText("ON");
    powerButton.setClickingTogglesState(true);
    powerAttachment = std::make_unique<XiffectsAudioProcessor::APVTS::ButtonAttachment>(processor.parameters,"POWER",powerButton);
    styleSlider(outputSlider);
    outputSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(outputSlider);
    outputAttachment=std::make_unique<Attachment>(processor.parameters,"OUTPUT",outputSlider);
    for(size_t i : {size_t(0),size_t(6),size_t(8)}) {
        modules[i]->bypass.setEnabled(false);
        modules[i]->bypass.setTooltip("DSP pending: this module is not yet available");
        modules[i]->titleLabel.setText(modules[i]->title + " *",juce::dontSendNotification);
        for(auto& slider:modules[i]->sliders) slider->setEnabled(false);
    }
    tuneStrength.setEnabled(false); tuneSpeed.setEnabled(false);
    resized();
}

void XiffectsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(bg);
    g.addTransform(juce::AffineTransform::scale(float(getWidth())/1440.0f, float(getHeight())/900.0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff27282b),0,0, bg,0,100,false));
    g.fillRect(0,0,1440,100);
    g.drawImage(hero,juce::Rectangle<float>(0,100,1440,302));
    const float moduleW=1396.0f/9;
    for(int i=0;i<9;++i) {
        juce::Rectangle<float> rect(22+float(i)*moduleW,402,moduleW-2,322);
        g.setGradientFill(juce::ColourGradient(panel,rect.getX(),402,juce::Colour(0xff0c0d10),rect.getX(),724,false));
        g.fillRoundedRectangle(rect,5);
        g.setColour(juce::Colour(0xff45464b)); g.drawRoundedRectangle(rect,5,1);
        g.setColour(red);
        const float cx=rect.getCentreX();
        for(int j=0;j<5;++j) g.drawVerticalLine(int(cx)+j*5-10,420.0f+float(std::abs(j-2)*4),446.0f-float(std::abs(j-2)*4));
    }
    g.setColour(panel); g.fillRoundedRectangle(22,736,1396,144,10);
    g.setColour(juce::Colour(0xff45464b));g.drawRoundedRectangle(22,736,1396,144,10,1);
    g.setColour(soft);g.setFont(juce::Font(11.0f));
    g.drawText("* OCTAVER / PITCH / CORRECTION DSP PENDING",900,750,490,20,juce::Justification::centredRight);
    g.drawText("NOTE MAP: SCALE INTERSECTED WITH MANUAL NOTES",310,755,550,20,juce::Justification::left);
    g.drawText("OUTPUT",1020,54,70,18,juce::Justification::left);
    auto meter=[&](int x,float peak,const juce::String& title) {
        g.setColour(soft);g.drawText(title,x,851,35,16,juce::Justification::left);
        g.setColour(juce::Colour(0xff37383d));g.fillRect(x+38,856,130,5);
        const float normalized=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(peak,-60.0f)+60)/60);
        g.setColour(peak>=1?juce::Colours::orange:red);g.fillRect(x+38,856,int(130*normalized),5);
    };
    meter(920,processor.inputPeak.load(),"IN"); meter(1140,processor.outputPeak.load(),"OUT");
}

void XiffectsAudioProcessorEditor::resized()
{
    brand.setBounds(48,20,230,50);
    byline.setBounds(290,30,230,30);
    saxLabel.setBounds(565,30,300,30);
    presetBox.setBounds(1000,20,220,30);
    powerButton.setBounds(1330,25,55,35);
    outputSlider.setBounds(1090,52,215,38);
    const int left=22, moduleW=1396/9;
    for(int i=0;i<int(modules.size());++i) {
        auto& m=*modules[size_t(i)]; const int x=left+i*moduleW;
        m.titleLabel.setBounds(x+4,451,moduleW-8,25);
        m.bypass.setBounds(x+moduleW-26,410,20,20);
        const int count=int(m.sliders.size());
        for(int j=0;j<count;++j) {
            if(j==0) {
                m.sliders[size_t(j)]->setBounds(x+24,478,moduleW-48,120);
                m.captions[size_t(j)]->setBounds(x+4,599,moduleW-8,18);
            } else {
                const int cell=(moduleW-12)/(count-1), sx=x+6+(j-1)*cell;
                m.sliders[size_t(j)]->setBounds(sx,625,cell,72);
                m.captions[size_t(j)]->setBounds(sx,699,cell,18);
            }
        }
    }
    tuneTitle.setBounds(42,749,250,25);
    keyLabel.setBounds(42,789,50,18); scaleLabel.setBounds(150,789,70,18);
    keyBox.setBounds(42,811,90,28); scaleBox.setBounds(150,811,135,28);
    for(int i=0;i<12;++i) noteButtons[size_t(i)].setBounds(310+i*45,793,45,48);
    strengthLabel.setBounds(900,784,150,20); tuneStrength.setBounds(1045,780,80,65);
    speedLabel.setBounds(1150,784,100,20); tuneSpeed.setBounds(1255,780,80,65);
    const auto transform=juce::AffineTransform::scale(float(getWidth())/1440.0f,float(getHeight())/900.0f);
    for(int i=0;i<getNumChildComponents();++i) getChildComponent(i)->setTransform(transform);
}
