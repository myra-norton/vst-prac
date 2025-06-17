#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), meter(p.levelL, p.levelR)
{
    juce::ignoreUnused (processorRef);

    tempoSyncButton.setButtonText ("Sync");
    tempoSyncButton.setClickingTogglesState (true);
    tempoSyncButton.setBounds (0,0,70,27);
    tempoSyncButton.setLookAndFeel (ButtonLookAndFeel::get());

    delayGroup.setText("Delay");
    delayGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    delayGroup.addAndMakeVisible (delayTimeKnob);
    delayGroup.addChildComponent (delayNoteKnob);
    delayGroup.addAndMakeVisible (tempoSyncButton);
    addAndMakeVisible (delayGroup);

    feedbackGroup.setText("Feedback");
    feedbackGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    feedbackGroup.addAndMakeVisible (feedbackKnob);
    feedbackGroup.addAndMakeVisible (stereoKnob);
    feedbackGroup.addAndMakeVisible (lowCutKnob);
    feedbackGroup.addAndMakeVisible (highCutKnob);
    addAndMakeVisible (feedbackGroup);

    outputGroup.setText("Output");
    outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    outputGroup.addAndMakeVisible (gainKnob);
    outputGroup.addAndMakeVisible (mixKnob);
    outputGroup.addAndMakeVisible (meter);
    addAndMakeVisible (outputGroup);

    auto bypassIcon = juce::ImageCache::getFromMemory (BinaryData::Bypass_png, BinaryData::Bypass_pngSize);
    bypassButton.setClickingTogglesState (true);
    bypassButton.setBounds (0,0,20,20);
    bypassButton.setImages (
        false, true, true,
        bypassIcon, 1.0f, juce::Colours::white,
        bypassIcon, 1.0f, juce::Colours::white,
        bypassIcon, 1.0f, juce::Colours::grey,
        0.0f);
    addAndMakeVisible (bypassButton);


    // addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    // inspectButton.onClick = [&] {
    //     if (!inspector)
    //     {
    //         inspector = std::make_unique<melatonin::Inspector> (*this);
    //         inspector->onClose = [this]() { inspector.reset(); };
    //     }
    //
    //     inspector->setVisible (true);
    // };

    setLookAndFeel (&mainLF);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 330);
    updateDelayKnobs(processorRef.params.tempoSyncParam->get());
    processorRef.params.tempoSyncParam->addListener (this);
}

PluginEditor::~PluginEditor()
{
    processorRef.params.tempoSyncParam->removeListener (this);
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    auto noise = juce::ImageCache::getFromMemory(
    BinaryData::Noise_png, BinaryData::Noise_pngSize);
    auto fillType = juce::FillType(noise, juce::AffineTransform::scale(0.5f));
    g.setFillType(fillType);
    g.fillRect(getLocalBounds());

    auto rect = getLocalBounds().withHeight(40);
    g.setColour(Colors::header);
    g.fillRect(rect);

    auto image = juce::ImageCache::getFromMemory(
        BinaryData::Logo_png, BinaryData::Logo_pngSize);

    int destWidth = image.getWidth() / 2;
    int destHeight = image.getHeight() / 2;
    g.drawImage(image,
                getWidth() / 2 - destWidth / 2, 0, destWidth, destHeight,
                0, 0, image.getWidth(), image.getHeight());
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();

    int y = 50;
    int height = bounds.getHeight()-60;

    delayGroup.setBounds (10, y, 110, height);
    outputGroup.setBounds(bounds.getWidth() - 160, y, 150, height);
    feedbackGroup.setBounds(delayGroup.getRight()+10, y,
        outputGroup.getX()-delayGroup.getRight()-20, height);

    delayTimeKnob.setTopLeftPosition (20, 20);
    tempoSyncButton.setTopLeftPosition (20, delayTimeKnob.getBottom()+10);
    delayNoteKnob.setTopLeftPosition (delayTimeKnob.getX(), delayTimeKnob.getY());
    mixKnob.setTopLeftPosition (20, 20);
    gainKnob.setTopLeftPosition (mixKnob.getX(), mixKnob.getBottom()+10);
    feedbackKnob.setTopLeftPosition (20, 20);
    stereoKnob.setTopLeftPosition (feedbackKnob.getRight()+20, 20);
    lowCutKnob.setTopLeftPosition (feedbackKnob.getX(), feedbackKnob.getBottom()+10);
    highCutKnob.setTopLeftPosition (lowCutKnob.getRight()+20, lowCutKnob.getY());
    meter.setBounds (outputGroup.getWidth() - 45, 30, 30, gainKnob.getBottom() - 30);
    bypassButton.setTopLeftPosition (bounds.getRight() - bypassButton.getWidth() - 10, 10);
    // layout the positions of your child components here
    // auto area = getLocalBounds();
    // area.removeFromBottom(50);
    // inspectButton.setBounds (getLocalBounds().withSizeKeepingCentre(100, 50));
}

void PluginEditor::parameterValueChanged (int, float value)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        updateDelayKnobs (value != 0.0f);
    } else {
        juce::MessageManager::callAsync([this, value] {
            updateDelayKnobs (value != 0.0f);
        });
    }
}

void PluginEditor::updateDelayKnobs (bool tempoSyncActive)
{
    delayTimeKnob.setVisible(!tempoSyncActive);
    delayNoteKnob.setVisible(tempoSyncActive);
}


