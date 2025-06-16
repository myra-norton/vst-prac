#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "Parameters.h"
#include "RotaryKnob.h"
#include "LookAndFeel.h"
#include "LevelMeter.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
private juce::AudioProcessorParameter::Listener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterValueChanged(int, float) override;
    void parameterGestureChanged(int, bool) override{}
    void updateDelayKnobs(bool tempoSyncActive);
    PluginProcessor& processorRef;
    MainLookAndFeel mainLF;

    RotaryKnob gainKnob {"Gain", processorRef.apvts, gainParamID, true};
    RotaryKnob mixKnob {"Mix", processorRef.apvts, mixParamID};
    RotaryKnob delayTimeKnob {"Time", processorRef.apvts, delayTimeParamID};
    RotaryKnob feedbackKnob {"Feedback", processorRef.apvts, feedbackParamID, true};
    RotaryKnob stereoKnob {"Stereo", processorRef.apvts, stereoParamID, true};
    RotaryKnob lowCutKnob {"Low Cut", processorRef.apvts, lowCutParamID};
    RotaryKnob highCutKnob {"High Cut", processorRef.apvts, highCutParamID};
    RotaryKnob delayNoteKnob {"Note", processorRef.apvts, delayNoteParamID};
    juce::TextButton tempoSyncButton;
    juce::AudioProcessorValueTreeState::ButtonAttachment tempoSyncAttachment {
        processorRef.apvts, tempoSyncParamID.getParamID(), tempoSyncButton
    };
    juce::GroupComponent delayGroup, feedbackGroup, outputGroup;

    LevelMeter meter;

    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
