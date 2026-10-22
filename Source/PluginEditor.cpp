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
    setLookAndFeel(&lookAndFeel);
    hero = juce::ImageCache::getFromMemory(BinaryData::hero_jpg, BinaryData::hero_jpgSize);
#if JUCE_IOS || JUCE_ANDROID
    setResizable(true,false);
    setSize(390,700);
#else
    setResizable(true, true);
    setResizeLimits(1152, 720, 1920, 1200);
    if (auto* constrainer = getConstrainer()) constrainer->setFixedAspectRatio(1.6);
    setSize (1440, 900);
#endif
    startTimerHz(24);
    brand.setText (p.isLiveMode()?"Xiffects LIVE":"Xiffects", juce::dontSendNotification);
    brand.setFont (juce::Font (p.isLiveMode()?30.0f:42.0f, juce::Font::plain));
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

    const auto& bank=XiffectsAudioProcessor::factoryPresets();
    for(int i=0;i<bank.size();++i) presetBox.addItem(bank[i]["category"].toString()+" / "+bank[i]["name"].toString(),i+1);
    presetBox.setText("Factory presets",juce::dontSendNotification);
    presetBox.onChange=[this] {processor.loadFactoryPreset(presetBox.getSelectedId()-1);};
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
            const auto& id=m->parameterIds[j];
            if(id.contains("MIX") || id.contains("STRENGTH") || id.contains("DEPTH") || id.contains("FEEDBACK")) s->setTextValueSuffix(" %");
            if(id=="COMP_THRESHOLD" || id=="COMP_MAKEUP" || id.startsWith("EQ_")) s->setTextValueSuffix(" dB");
            if(id=="DLY_TIME" || id=="REV_DECAY") s->setTextValueSuffix(" s");
            if(id=="PHASER_RATE") s->setTextValueSuffix(" Hz");
            if(id=="TUNE_SPEED") s->setTextValueSuffix(" ms");
            if(id=="PITCH_SEMITONES") s->setTextValueSuffix(" st");
            if(i==5) s->setSliderStyle(juce::Slider::LinearVertical);
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
    for(size_t i=0;i<xiffects::scales.size();++i) scaleBox.addItem(xiffects::scales[i].name,int(i)+1);
    styleCombo (keyBox); styleCombo (scaleBox);
    addAndMakeVisible (keyBox); addAndMakeVisible (scaleBox);
    keyAttachment = std::make_unique<ComboAttachment> (processor.parameters, "TUNE_KEY", keyBox);
    scaleBox.setSelectedId(processor.selectedScale()+1,juce::dontSendNotification);
    scaleBox.onChange=[this] {
        const int index=scaleBox.getSelectedId()-1;
        auto set=[this](const char* id,int value) {auto* p=processor.parameters.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(value)));p->endChangeGesture();};
        if(index<8) {set("TUNE_SCALE",index);set("TUNE_SCALE_EXT",0);}
        else set("TUNE_SCALE_EXT",index-7);
    };

    for (int i = 0; i < 12; ++i)
    {
        const juce::String noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        noteButtons[(size_t)i].setButtonText (noteNames[i]);
        noteButtons[(size_t)i].setClickingTogglesState (true);
        noteButtons[(size_t)i].setToggleState (true, juce::dontSendNotification);
        noteButtons[(size_t)i].accidental=(i==1 || i==3 || i==6 || i==8 || i==10);
        noteButtons[(size_t)i].setTooltip("Toggle manual permission. Red means allowed by both key/scale and this switch.");
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
    tuneSpeed.setTextValueSuffix(" ms"); tuneStrength.setTextValueSuffix(" %");
    for(auto* button:{&saveButton,&loadButton,&settingsButton}) addAndMakeVisible(*button);
    saveButton.onClick=[this] {choosePresetFile(true);};
    loadButton.onClick=[this] {choosePresetFile(false);};
    settingsButton.onClick=[this] {showSettings();};
#if JUCE_IOS || JUCE_ANDROID
    touchPage.addItemList({"Effects","Tuning","Monitor"},1);
    touchModule.addItemList({"Octaver","Compressor","Reverb","Delay","Distortion","EQ","Pitch","Phaser","Auto Tune"},1);
    for(auto* box:{&touchPage,&touchModule}) {styleCombo(*box);addAndMakeVisible(*box);box->setSelectedId(1);box->onChange=[this]{resized();repaint();};}
#endif
    timerCallback();
    resized();
}

void XiffectsAudioProcessorEditor::paint (juce::Graphics& g)
{
#if JUCE_IOS || JUCE_ANDROID
    paintTouch(g);return;
#endif
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
    g.drawText("PITCH: DETECTED / APPLIED ESTIMATE",850,750,290,20,juce::Justification::centredLeft);
    g.drawText("ALLOWED NOTES  /  SCALE + MANUAL",310,755,530,20,juce::Justification::left);
    g.drawText("OUTPUT",960,62,70,18,juce::Justification::left);
    auto meter=[&](int x,float peak,const juce::String& title) {
        g.setColour(soft);g.drawText(title,x,851,35,16,juce::Justification::left);
        g.setColour(juce::Colour(0xff37383d));g.fillRect(x+38,856,130,5);
        const float normalized=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(peak,-60.0f)+60)/60);
        g.setColour(peak>=1?juce::Colours::orange:red);g.fillRect(x+38,856,int(130*normalized),5);
    };
    meter(45,displayInput,"IN"); meter(240,displayOutput,"OUT");
    const juce::Rectangle<float> graph(850,780,275,83);
    g.setColour(juce::Colour(0xff0d0e11)); g.fillRect(graph);
    for(int note=48;note<=96;note+=12) {
        const float y=graph.getBottom()-(float(note)-48)/48*graph.getHeight();
        g.setColour(juce::Colour(0xff303238));g.drawHorizontalLine(int(y),graph.getX(),graph.getRight());
    }
    for(int trace=0;trace<2;++trace) {
        juce::Path path;bool pen=false;
        for(size_t i=0;i<history.size();++i) {
            const auto& frame=history[(historyPosition+i)%history.size()];
            const float note=trace==0?frame.detected:frame.corrected;
            if(note<=0) {pen=false;continue;}
            const float x=graph.getX()+float(i)*graph.getWidth()/float(history.size()-1);
            const float y=graph.getBottom()-juce::jlimit(0.f,1.f,(note-48)/48)*graph.getHeight();
            if(pen) path.lineTo(x,y);else path.startNewSubPath(x,y);
            pen=true;
        }
        g.setColour(trace==0?juce::Colour(0xffb9bdc7):red);g.strokePath(path,juce::PathStrokeType(1.3f));
    }
    // Genuine output min/max envelope, not a synthetic animation.
    g.setColour(red.withAlpha(0.65f));
    for(size_t i=0;i<history.size();++i) {
        const auto& frame=history[(historyPosition+i)%history.size()];
        g.drawVerticalLine(450+int(i),859-frame.maximum*12,859-frame.minimum*12+0.5f);
    }
}

void XiffectsAudioProcessorEditor::resized()
{
#if JUCE_IOS || JUCE_ANDROID
    layoutTouch();return;
#endif
    brand.setBounds(48,20,230,50);
    byline.setBounds(290,30,230,30);
    saxLabel.setBounds(565,30,300,30);
    presetBox.setBounds(960,20,220,30);
    saveButton.setBounds(1195,22,48,27); loadButton.setBounds(1248,22,48,27); settingsButton.setBounds(1301,22,30,27);
    powerButton.setBounds(1340,25,55,35);
    outputSlider.setBounds(1030,56,280,38);
    const int left=22, moduleW=1396/9;
    for(int i=0;i<int(modules.size());++i) {
        auto& m=*modules[size_t(i)]; const int x=left+i*moduleW;
        m.titleLabel.setBounds(x+4,451,moduleW-8,25);
        m.bypass.setBounds(x+moduleW-26,410,20,20);
        const int count=int(m.sliders.size());
        for(int j=0;j<count;++j) {
            if(i==5) {
                const int cell=(moduleW-12)/4;
                m.sliders[size_t(j)]->setBounds(x+6+j*cell,495,cell,192);
                m.sliders[size_t(j)]->setTextBoxStyle(juce::Slider::TextBoxBelow,false,cell,30);
                m.captions[size_t(j)]->setBounds(x+6+j*cell,695,cell,20);
            } else if(j==0) {
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
    const int whiteNotes[]={0,2,4,5,7,9,11};
    for(int i=0;i<7;++i) noteButtons[size_t(whiteNotes[i])].setBounds(310+i*74,788,74,57);
    const int blackNotes[]={1,3,6,8,10}, offsets[]={1,2,4,5,6};
    for(int i=0;i<5;++i) {
        auto& key=noteButtons[size_t(blackNotes[i])];
        key.setBounds(310+offsets[i]*74-22,788,44,37); key.toFront(false);
    }
    strengthLabel.setBounds(1145,780,170,18); tuneStrength.setBounds(1320,765,70,55);
    speedLabel.setBounds(1145,831,130,18); tuneSpeed.setBounds(1320,820,70,55);
    const auto transform=juce::AffineTransform::scale(float(getWidth())/1440.0f,float(getHeight())/900.0f);
    for(int i=0;i<getNumChildComponents();++i)
        if(dynamic_cast<juce::ResizableCornerComponent*>(getChildComponent(i))==nullptr)
            getChildComponent(i)->setTransform(transform);
}

XiffectsAudioProcessorEditor::~XiffectsAudioProcessorEditor() {
    stopTimer(); setLookAndFeel(nullptr);
}

void XiffectsAudioProcessorEditor::timerCallback() {
    xiffects::TelemetryFrame frame;
    while(processor.telemetry.pop(frame)) {
        history[historyPosition]=frame; historyPosition=(historyPosition+1)%history.size();
    }
    displayInput=std::max(processor.inputPeak.load(),displayInput*0.88f);
    displayOutput=std::max(processor.outputPeak.load(),displayOutput*0.88f);
    std::array<bool,12> all;all.fill(true);
    const int key=int(processor.parameters.getRawParameterValue("TUNE_KEY")->load());
    const int scale=processor.selectedScale();
    scaleBox.setSelectedId(scale+1,juce::dontSendNotification);
    for(size_t i=0;i<noteButtons.size();++i) {
        noteButtons[i].inScale=xiffects::allowed(int(i),key,scale,all);noteButtons[i].repaint();
    }
    repaint();
}

void XiffectsAudioProcessorEditor::choosePresetFile(bool save) {
    fileChooser=std::make_unique<juce::FileChooser>(save?"Save Xiffects preset":"Load Xiffects preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.xiffects");
    const int flags=save?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting)
                        :(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
    const juce::Component::SafePointer<XiffectsAudioProcessorEditor> safe(this);
    fileChooser->launchAsync(flags,[safe,save](const juce::FileChooser& chooser) {
        if(safe==nullptr) return;
        auto file=chooser.getResult();if(file==juce::File()) return;
        bool ok=false;
        if(save) {
            file=file.withFileExtension("xiffects");
            auto xml=safe->processor.parameters.copyState().createXml();
            ok=xml!=nullptr && xml->writeTo(file);
        } else if(file.getSize()<1024*1024) {
            auto xml=juce::XmlDocument::parse(file);
            if(xml!=nullptr && xml->hasTagName(safe->processor.parameters.state.getType())) {
                const auto state=XiffectsAudioProcessor::migrateState(juce::ValueTree::fromXml(*xml));
                // Validate first, then apply complete parameter values through host gestures.
                ok=state.isValid() && state.getNumChildren()>0;
                for(const auto child:state) {
                    auto* parameter=safe->processor.parameters.getParameter(child["id"].toString());
                    const float value=float(child["value"]);
                    if(parameter==nullptr || !std::isfinite(value)) {ok=false;break;}
                }
                if(ok) for(const auto child:state) {
                    auto* parameter=safe->processor.parameters.getParameter(child["id"].toString());
                    parameter->beginChangeGesture();
                    parameter->setValueNotifyingHost(juce::jlimit(0.f,1.f,parameter->convertTo0to1(float(child["value"]))));
                    parameter->endChangeGesture();
                }
            }
        }
        if(ok) safe->presetBox.setText(file.getFileNameWithoutExtension(),juce::dontSendNotification);
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset",save?"Could not save the preset.":"Invalid or unreadable Xiffects preset.");
    });
}

void XiffectsAudioProcessorEditor::showSettings() {
    juce::PopupMenu menu;
    menu.addSectionHeader("INTERFACE SIZE");
    menu.addItem(1,"80% — compact");menu.addItem(2,"100% — standard");menu.addItem(3,"120% — large");
    menu.addSeparator();
    menu.addItem(4,"About processing and latency");
    const juce::Component::SafePointer<XiffectsAudioProcessorEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton),[safe](int choice) {
        if(safe==nullptr) return;
        if(choice>=1 && choice<=3) {
            const float scale=0.6f+0.2f*float(choice);
            safe->setSize(int(1440*scale),int(900*scale));
        }
        if(choice==4) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
            "Xiffects — Unique Sounds by Xivan",
            "Monophonic pitch analysis: 100–1600 Hz. Use a clean, isolated sax input.\n"
            + juce::String(safe->processor.isLiveMode()?
            "LIVE EXPERIMENTAL: nominal plugin delay about 12 ms, including bypass. " :
            "STUDIO: nominal plugin delay about 60 ms, including bypass. ") +
            "Shorter Live grains may colour attacks and low notes. "
            "The host is notified. Analysis needs additional settling time.\n"
            "Red graph: applied correction estimate, not a second output detector. "
            "Granular shifting can colour attacks, vibrato and breath noise. No formant preservation.\n"
            "Preset files store all parameters. Red piano indicators show the intersection of scale and manual permission.");
    });
}

#if JUCE_IOS || JUCE_ANDROID
void XiffectsAudioProcessorEditor::layoutTouch() {
    for(int i=0;i<getNumChildComponents();++i) {auto* c=getChildComponent(i);c->setTransform({});c->setVisible(false);}
    const int w=getWidth(), page=touchPage.getSelectedId();
    auto place=[](juce::Component& c,int x,int y,int width,int height){c.setVisible(true);c.setBounds(x,y,width,height);};
    place(brand,12,4,w-80,44);brand.setFont(juce::Font(26.f));
    place(powerButton,w-60,4,48,44);
    place(presetBox,12,52,w-24,44);
    place(saveButton,12,102,68,44);place(loadButton,84,102,68,44);
    place(settingsButton,156,102,48,44);place(touchPage,210,102,w-222,44);
    const int y=154;
    if(page==1) {
        place(touchModule,12,y,w-24,44);
        const int index=juce::jlimit(0,8,touchModule.getSelectedId()-1);
        if(index<int(modules.size())) {
            auto& m=*modules[size_t(index)];place(m.bypass,w-104,y+50,92,44);
            place(m.titleLabel,12,y+50,w-128,44);
            const int width=(w-36)/2;
            for(size_t j=0;j<m.sliders.size();++j) {
                const int x=12+int(j%2)*(width+12), top=y+100+int(j/2)*126;
                m.sliders[j]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                m.sliders[j]->setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,30);
                place(*m.sliders[j],x,top,width,100);place(*m.captions[j],x,top+100,width,24);
            }
        }
    } else if(page==2) {
        place(keyBox,12,y,82,44);place(scaleBox,102,y,w-114,44);
        place(strengthLabel,12,y+50,w/2-18,30);place(speedLabel,w/2,y+50,w/2-12,30);
        place(tuneStrength,12,y+82,w/2-18,88);place(tuneSpeed,w/2,y+82,w/2-12,88);
        for(size_t i=0;i<12;++i)place(noteButtons[i],12+int(i%6)*(w-24)/6,y+184+int(i/6)*56,(w-24)/6-3,52);
    } else {
        outputSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        place(outputSlider,12,y,w-24,60);
    }
}
void XiffectsAudioProcessorEditor::paintTouch(juce::Graphics& g) {
    g.fillAll(bg);
    if(touchPage.getSelectedId()!=3) {
        if(getHeight()>650)g.drawImageWithin(hero,12,getHeight()-120,getWidth()-24,108,juce::RectanglePlacement::centred);
        return;
    }
    const float width=float(getWidth()-24);
    g.setColour(soft);g.drawText("OUTPUT / INPUT & OUTPUT METERS",12,224,getWidth()-24,24,juce::Justification::left);
    for(int i=0;i<2;++i) {
        const float peak=i==0?displayInput:displayOutput;
        g.setColour(panel);g.fillRect(12.f,256.f+i*24,width,12.f);
        g.setColour(red);g.fillRect(12.f,256.f+i*24,width*juce::jlimit(0.f,1.f,(juce::Decibels::gainToDecibels(peak,-60.f)+60)/60),12.f);
    }
    const auto area=juce::Rectangle<float>(12,320,width,100);
    for(int trace=0;trace<2;++trace) {
        juce::Path path;bool pen=false;
        for(size_t i=0;i<history.size();++i) {
            const auto& frame=history[(historyPosition+i)%history.size()];float note=trace==0?frame.detected:frame.corrected;
            if(note<=0){pen=false;continue;}
            float x=area.getX()+float(i)*width/float(history.size()), y=area.getBottom()-juce::jlimit(0.f,1.f,(note-48)/48)*100;
            if(pen)path.lineTo(x,y);else path.startNewSubPath(x,y);pen=true;
        }
        g.setColour(trace==0?soft:red);g.strokePath(path,juce::PathStrokeType(1.5f));
    }
    g.setColour(soft);g.drawText("PITCH: INPUT / APPLIED ESTIMATE",12,294,getWidth()-24,22,juce::Justification::left);
    g.setColour(red);
    for(size_t i=0;i<history.size();++i) {
        const auto& f=history[(historyPosition+i)%history.size()];
        g.drawVerticalLine(12+int(float(i)*width/float(history.size())),470-f.maximum*30,470-f.minimum*30+.5f);
    }
}
#endif
