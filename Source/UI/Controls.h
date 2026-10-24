#pragma once
#include <JuceHeader.h>

namespace xiffects {
class LookAndFeel final : public juce::LookAndFeel_V4 {
public:
    LookAndFeel() {
        setColour(juce::TextButton::buttonColourId,juce::Colour(0xff202126));
        setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xffaf1536));
        setColour(juce::TextButton::textColourOffId,juce::Colour(0xffdddddf));
        setColour(juce::TextButton::textColourOnId,juce::Colours::white);
    }
    juce::Font getTextButtonFont(juce::TextButton&,int height) override {return juce::Font(juce::jmin(15.f,float(height)*.65f));}
    juce::Font getComboBoxFont(juce::ComboBox&) override {return juce::Font(15.f);}
    juce::Font getPopupMenuFont() override {return juce::Font(15.f);}
    juce::Font getLabelFont(juce::Label& label) override {
        if(dynamic_cast<juce::Slider*>(label.getParentComponent()))return juce::Font(15.f);
        return label.getFont();
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float value,
                          float start,float end,juce::Slider&) override {
        const auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height)).reduced(5);
        const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*0.5f;
        const auto centre=bounds.getCentre();
        const float angle=start+(end-start)*value;
        juce::Path track, active;
        track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
        active.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
        g.setColour(juce::Colour(0xff47494e));g.strokePath(track,juce::PathStrokeType(1.3f));
        g.setColour(juce::Colour(0xffff244a).withAlpha(0.15f));g.strokePath(active,juce::PathStrokeType(6));
        g.setColour(juce::Colour(0xffff3153));g.strokePath(active,juce::PathStrokeType(2));
        const float r=radius*0.79f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff45464c),centre.x-r,centre.y-r,
            juce::Colour(0xff090a0c),centre.x+r,centre.y+r,false));
        g.fillEllipse(centre.x-r,centre.y-r,r*2,r*2);
        g.setColour(juce::Colour(0xff55575d));g.drawEllipse(centre.x-r,centre.y-r,r*2,r*2,0.7f);
        g.setColour(juce::Colour(0xffff6780));
        g.drawLine(centre.x+std::sin(angle)*r*0.6f,centre.y-std::cos(angle)*r*0.6f,
                   centre.x+std::sin(angle)*r*0.91f,centre.y-std::cos(angle)*r*0.91f,1.8f);
    }
};
class NoteKey final : public juce::Button {
public:
    NoteKey():juce::Button(juce::String()) {setClickingTogglesState(true);}
    bool accidental=false, inScale=true;
    void paintButton(juce::Graphics& g,bool hover,bool down) override {
        auto bounds=getLocalBounds().toFloat().reduced(1);
        const bool enabled=getToggleState() && inScale;
        auto colour=accidental?juce::Colour(0xff16171b):juce::Colour(0xff777a82);
        if(hover||down) colour=colour.brighter(0.2f);
        g.setGradientFill(juce::ColourGradient(colour,0,0,colour.darker(0.5f),0,float(getHeight()),false));
        g.fillRoundedRectangle(bounds,2);
        g.setColour(enabled?juce::Colour(0xffff3153):juce::Colour(0xff36383d));
        g.fillRect(bounds.removeFromBottom(5));
        if(!getToggleState()) {g.setColour(juce::Colours::black.withAlpha(0.4f));g.fillRoundedRectangle(bounds,2);}
        g.setColour(enabled?juce::Colours::white:juce::Colour(0xffb8bac1));
        g.setFont(juce::Font(14));
        g.drawText(getButtonText(),getLocalBounds().reduced(1).removeFromBottom(22),juce::Justification::centred);
    }
};
}
