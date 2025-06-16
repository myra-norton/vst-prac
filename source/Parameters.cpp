//
// Created by Myra Norton on 6/10/25.
//
#include "Parameters.h"
#include "DSP.h"

template<typename T>
static void castParameter(juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id, T& destination)
{
    destination = dynamic_cast<T>(apvts.getParameter(id.getParamID()));
    jassert(destination);
}

static juce::String stringFromMilliseconds(float value, int)
{
    if (value < 10.0f){
        return juce::String(value, 2) + " ms";
    } else if (value < 100.0f) {
        return juce::String(value, 1) + " ms";
    } else if (value < 1000.0f) {
        return juce::String(int(value)) + " ms";
    } else {
        return juce::String(value * 0.001f, 2) + " s";
    }
}

static juce::String stringFromDecibels(float value, int)
{
    return juce::String(value, 1) + " dB";
}

static juce::String stringFromPercent(float value, int)
{
    return juce::String(int(value)) + " %";
}

static juce::String stringFromHz(float value, int)
{
    if (value < 1000.0f) {
        return juce::String(int(value)) + " Hz";
    } else if (value < 10000.0f) {
        return juce::String(value / 1000.0f, 2) + " k";
    } else {
        return juce::String(value / 1000.0f, 1) + " k";
    }
}

static float hzFromString(const juce::String& str)
{
    float value = str.getFloatValue();
    if (value < 20.0f) {
        return value * 1000.0f;
    }
    return value;
}

static float millisecondsFromString(const juce::String& text)
{
    float value = text.getFloatValue();
    if (!text.endsWithIgnoreCase ("ms"))
    {
        if (text.endsWithIgnoreCase ("s") || value < Parameters::minDelayTime)
        {
            return value * 1000.0f;
        }
    }
    return value;
}

// creates our Parameters object
Parameters::Parameters(juce::AudioProcessorValueTreeState& apvts)
{
    castParameter (apvts, gainParamID, gainParam);
    castParameter (apvts, delayTimeParamID, delayTimeParam);
    castParameter (apvts, mixParamID, mixParam);
    castParameter (apvts, feedbackParamID, feedbackParam);
    castParameter (apvts, stereoParamID, stereoParam);
    castParameter (apvts, lowCutParamID, lowCutParam);
    castParameter (apvts, highCutParamID, highCutParam);
    castParameter (apvts, tempoSyncParamID, tempoSyncParam);
    castParameter (apvts, delayNoteParamID, delayNoteParam);
}

// the function fills out the ParameterLayout object and returns it
// the purpos of the ParameterLayout object is to describe the
// parameter objects that should be added to the APVTS
juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createParameterLayout()
{
    // create the layout object
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // add a parameter (AudioParameterFloat object) to the layout
    // to make an AudioParameterFloat object you need 4 arguments
        // a parameter ID (name and version hint)
        // name of the parameter
        // range of the parameter as a juce::NormalisableRange object
        // default value
    layout.add(std::make_unique<juce::AudioParameterFloat>(gainParamID,
        "Output Gain", juce::NormalisableRange<float>{-12.0f, 12.0f}, 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromDecibels)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(delayTimeParamID,
        "Delay Time", juce::NormalisableRange<float>{minDelayTime, maxDelayTime, 0.001f, 0.25f}, 100.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromMilliseconds)
        .withValueFromStringFunction (millisecondsFromString)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(mixParamID, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(feedbackParamID, "Feedback",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(stereoParamID,"Stereo",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (stringFromPercent)));
    layout.add(std::make_unique<juce::AudioParameterFloat> (lowCutParamID, "Low Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (stringFromHz)
        .withValueFromStringFunction (hzFromString)));
    layout.add(std::make_unique<juce::AudioParameterFloat> (highCutParamID, "High Cut",
        juce::NormalisableRange<float>(0.0f, 20000.0f, 1.0f, 0.3f), 20000.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (stringFromHz)
        .withValueFromStringFunction (hzFromString)));
    layout.add(std::make_unique<juce::AudioParameterBool>(tempoSyncParamID, "Tempo Sync", false));
    juce::StringArray noteLengths = {
        "1/32",
        "1/16 trip",
        "1/32 dot",
        "1/16",
        "1/8 trip",
        "1/16 dot",
        "1/8",
        "1/4 trip",
        "1/8 dot",
        "1/4",
        "1/2 trip",
        "1/4 dot",
        "1/2",
        "1/1 trip",
        "1/2 dot",
        "1/1",
    };
    layout.add(std::make_unique<juce::AudioParameterChoice>(delayNoteParamID, "Delay Note", noteLengths, 9));
    return layout;
}

void Parameters::update() noexcept
{
    gainSmoother.setTargetValue (juce::Decibels::decibelsToGain(gainParam->get()));
    targetDelayTime = delayTimeParam->get();
    if (delayTime == 0.0f) {
        delayTime = targetDelayTime;
    }
    mixSmoother.setTargetValue(mixParam->get() * 0.01f);
    feedbackSmoother.setTargetValue(feedbackParam->get() * 0.01f);
    stereoSmoother.setTargetValue(stereoParam->get() * 0.01f);
    lowCutSmoother.setTargetValue(lowCutParam->get());
    highCutSmoother.setTargetValue(highCutParam->get());
    delayNote = delayNoteParam->getIndex();
    tempoSync = tempoSyncParam->get();
}

void Parameters::prepareToPlay(double sampleRate) noexcept
{
    double duration = 0.02;
    gainSmoother.reset(sampleRate, duration);
    coeff = 1.0f - std::exp(-1.0f / (0.2f * float(sampleRate)));
    mixSmoother.reset(sampleRate, duration);
    feedbackSmoother.reset(sampleRate, duration);
    stereoSmoother.reset(sampleRate, duration);
    lowCutSmoother.reset(sampleRate, duration);
    highCutSmoother.reset(sampleRate, duration);
}

void Parameters::reset() noexcept
{
    gain = 0.0f;
    delayTime = 0.0f;
    mix = 1.0f;
    feedback = 0.0f;
    panL = 0.0f;
    panR = 1.0f;
    lowCut = 20.0f;
    highCut = 20000.0f;

    gainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gainParam->get()));
    mixSmoother.setCurrentAndTargetValue(mixParam->get() * 0.01f);
    feedbackSmoother.setCurrentAndTargetValue(feedbackParam->get() * 0.01f);
    stereoSmoother.setCurrentAndTargetValue(stereoParam->get() * 0.01f);
    lowCutSmoother.setCurrentAndTargetValue(lowCutParam->get());
    highCutSmoother.setCurrentAndTargetValue(highCutParam->get());
}

void Parameters::smoothen() noexcept
{
    gain = gainSmoother.getNextValue();
    delayTime += (targetDelayTime - delayTime) * coeff;
    mix = mixSmoother.getNextValue();
    feedback = feedbackSmoother.getNextValue();
    panningEqualPower (stereoSmoother.getNextValue(), panL, panR);
    lowCut = lowCutSmoother.getNextValue();
    highCut = highCutSmoother.getNextValue();
}
