#include "PluginEditor.h"

//==============================================================================
DarkTrapAudioProcessorEditor::DarkTrapAudioProcessorEditor (DarkTrapAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      midiDragBox (p)
{
    addAndMakeVisible (generateButton);
    generateButton.onClick = [this] { processorRef.generateMelody(); };

    addAndMakeVisible (presetBox);
    presetBox.addItemList ({ "Init", "Dark Bell", "Ghost Pluck", "Sub Bass", "Tape Keys" }, 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedItemIndex();

        if (index >= 0)
            processorRef.loadPreset (index);
    };

    addAndMakeVisible (loadSoundButton);
    loadSoundButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load SoundFont / SFZ / WAV samples",
            juce::File(),
            "*.sf2;*.sfz;*.wav");

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectFiles
                | juce::FileBrowserComponent::canSelectMultipleItems,
            [this] (const juce::FileChooser& fc)
            {
                const auto results = fc.getResults();

                if (! results.isEmpty())
                    processorRef.loadInstrumentBank (results);
            });
    };

    addAndMakeVisible (halfTimeButton);
    halfTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, "halfTime", halfTimeButton);

    configureSlider (cutoffSlider,  cutoffLabel,  "cutoff",  "Cutoff");
    configureSlider (attackSlider,  attackLabel,  "attack",  "Attack");
    configureSlider (decaySlider,   decayLabel,   "decay",   "Decay");
    configureSlider (sustainSlider, sustainLabel, "sustain", "Sustain");
    configureSlider (releaseSlider, releaseLabel, "release", "Release");
    configureSlider (reverbSlider,  reverbLabel,  "reverb",  "Reverb");
    configureSlider (delaySlider,   delayLabel,   "delay",   "Delay");
    configureSlider (wobbleSlider,  wobbleLabel,  "wobble",  "Tape Wobble");

    addAndMakeVisible (midiDragBox);

    setSize (820, 480);
}

void DarkTrapAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101010));
}

void DarkTrapAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto top = area.removeFromTop (40);

    generateButton.setBounds (top.removeFromLeft (140).reduced (0, 4));
    top.removeFromLeft (12);
    presetBox.setBounds (top.removeFromLeft (190).reduced (0, 4));
    top.removeFromLeft (12);
    loadSoundButton.setBounds (top.removeFromLeft (190).reduced (0, 4));
    top.removeFromLeft (12);
    halfTimeButton.setBounds (top.removeFromLeft (110).reduced (0, 4));

    area.removeFromTop (12);

    auto dragArea = area.removeFromRight (310);
    midiDragBox.setBounds (dragArea.removeFromTop (150));

    auto controls = area;

    const int sliderW = 95;
    const int sliderH = 95;
    const int labelH = 20;
    const int gap = 10;

    int x = controls.getX();
    int y = controls.getY();

    auto place = [&] (juce::Slider& s, juce::Label& l)
    {
        positionSlider (s, l, x, y, sliderW, sliderH + labelH);
        x += sliderW + gap;

        if (x + sliderW > controls.getRight())
        {
            x = controls.getX();
            y += sliderH + labelH + gap;
        }
    };

    place (cutoffSlider, cutoffLabel);
    place (reverbSlider, reverbLabel);
    place (delaySlider, delayLabel);
    place (wobbleSlider, wobbleLabel);

    x = controls.getX();
    y += sliderH + labelH + gap;

    place (attackSlider, attackLabel);
    place (decaySlider, decayLabel);
    place (sustainSlider, sustainLabel);
    place (releaseSlider, releaseLabel);
}

void DarkTrapAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                    juce::Label& label,
                                                    const juce::String& paramID,
                                                    const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.15f,
                                juce::MathConstants<float>::pi * 2.85f,
                                true);

    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (label);

    sliderAttachments.push_back (
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processorRef.apvts, paramID, slider));
}

void DarkTrapAudioProcessorEditor::positionSlider (juce::Slider& slider,
                                                   juce::Label& label,
                                                   int x, int y, int w, int h)
{
    label.setBounds (x, y, w, 20);
    slider.setBounds (x, y + 20, w, h - 20);
}
