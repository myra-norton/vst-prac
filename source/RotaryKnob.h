//
// Created by Myra Norton on 6/11/25.
//
#pragma once
// #include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class RotaryKnob  : public juce::Component
{
public:
    RotaryKnob(const juce::String& text, juce::AudioProcessorValueTreeState& apvts,
        const juce::ParameterID& parameterID, bool drawFromMiddle = false);
    ~RotaryKnob() override;

    void resized() override;

    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};
