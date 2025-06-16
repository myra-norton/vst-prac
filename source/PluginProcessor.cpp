#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProtectYourEars.h"

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (
         BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        ), params(apvts)
{
    lowCutFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    params.prepareToPlay (sampleRate);
    params.reset();
    feedbackL = 0.0f;
    feedbackR = 0.0f;
    lastLowCut = -1.0f;
    lastHighCut = -1.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32(samplesPerBlock);
    spec.numChannels = 2;
    double numSamples = Parameters::maxDelayTime/1000.0 * sampleRate;
    int maxDelayInSamples = int(std::ceil(numSamples));
    delayLineL.setMaximumDelayInSamples(maxDelayInSamples);
    delayLineR.setMaximumDelayInSamples(maxDelayInSamples);
    delayLineL.reset();
    delayLineR.reset();
    lowCutFilter.prepare(spec);
    lowCutFilter.reset();
    highCutFilter.prepare(spec);
    highCutFilter.reset();
    tempo.reset();
    levelL.reset();
    levelR.reset();
    // DBG(maxDelayInSamples);
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn == mono && mainOut == mono) {return true;}
    if (mainIn == stereo && mainOut == stereo) {return true;}
    if (mainIn == stereo && mainOut == stereo) {return true;}
    return false;
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, [[maybe_unused]]
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // write silence into the audio buffer
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    params.update();
    tempo.update(getPlayHead());

    float syncedTime = float(tempo.getMillisecondsForNoteLength (params.delayNote));
    if (syncedTime > Parameters::maxDelayTime) {
        syncedTime = Parameters::maxDelayTime;
    }

    float sampleRate = float(getSampleRate());

    auto mainInput = getBusBuffer(buffer, true, 0);
    auto mainInputChannels = mainInput.getNumChannels();
    auto isMainInputStereo = mainInputChannels > 1;
    const float* inputDataL = mainInput.getReadPointer(0);
    const float* inputDataR = mainInput.getReadPointer(isMainInputStereo ? 1 : 0);

    auto mainOutput = getBusBuffer(buffer, false, 0);
    auto mainOutputChannels = mainOutput.getNumChannels();
    auto isMainOutputStereo = mainOutputChannels > 1;
    float* outputDataL = mainOutput.getWritePointer(0);
    float* outputDataR = mainOutput.getWritePointer(isMainOutputStereo ? 1 : 0);

    float maxL = 0.0f;
    float maxR = 0.0f;

    if (isMainOutputStereo)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            params.smoothen();

            float delayTime = params.tempoSync ? syncedTime : params.delayTime;
            float delayInSamples = delayTime/1000.0f * sampleRate;

            if (params.lowCut != lastLowCut) {
                lowCutFilter.setCutoffFrequency (params.lowCut);
                lastLowCut = params.lowCut;
            }
            if (params.highCut != lastHighCut) {
                highCutFilter.setCutoffFrequency (params.highCut);
                lastHighCut = params.highCut;
            }

            float dryL = inputDataL[sample];
            float dryR = inputDataR[sample];

            float mono  = (dryL + dryR) * 0.5f;

            delayLineL.write (mono*params.panL + feedbackR);
            delayLineR.write (mono*params.panR + feedbackL);

            float wetL = delayLineL.read (delayInSamples);
            float wetR = delayLineR.read (delayInSamples);

            feedbackL = wetL * params.feedback;
            feedbackL = lowCutFilter.processSample (0, feedbackL);
            feedbackL = highCutFilter.processSample (0, feedbackL);
            feedbackR = wetR * params.feedback;
            feedbackR = lowCutFilter.processSample (1, feedbackR);
            feedbackR = highCutFilter.processSample (1, feedbackR);

            // multi-tap delay
            // wetL += delayLine.popSample(0, delayInSamples*2.0f, false) * 0.7f;
            // wetR += delayLine.popSample(0, delayInSamples*2.0f, false) * 0.7f;

            float mixL = dryL + wetL * params.mix;
            float mixR = dryR + wetR * params.mix;

            float outL = mixL * params.gain;
            float outR = mixR * params.gain;
            outputDataL[sample] = outL;
            outputDataR[sample] = outR;
            maxL = std::max(maxL, std::abs(outL));
            maxR = std::max(maxR, std::abs(outR));
        }
    } else {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            params.smoothen();

            float delayTime = params.tempoSync ? syncedTime : params.delayTime;
            float delayInSamples = delayTime/1000.0f * sampleRate;

            if (params.lowCut != lastLowCut) {
                lowCutFilter.setCutoffFrequency (params.lowCut);
                lastLowCut = params.lowCut;
            }
            if (params.highCut != lastHighCut) {
                highCutFilter.setCutoffFrequency (params.highCut);
                lastHighCut = params.highCut;
            }

            float dry = inputDataL[sample];
            delayLineL.write (dry + feedbackL);

            float wet = delayLineL.read (delayInSamples);
            feedbackL = wet * params.feedback;
            feedbackL = lowCutFilter.processSample (0, feedbackL);
            feedbackL = highCutFilter.processSample (0, feedbackL);

            float mix = dry + wet * params.mix;
            outputDataL[sample] = mix * params.gain;
            float outL = mix * params.gain;
            outputDataL[sample] = outL;
            maxL = std::max(maxL, std::abs(outL));
        }
    }
    levelL.updateIfGreater (maxL);
    levelR.updateIfGreater (maxR);
    #if JUCE_DEBUG
    protectYourEars (buffer);
    #endif
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
    // DBG(apvts.copyState().toXmlString ());
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
