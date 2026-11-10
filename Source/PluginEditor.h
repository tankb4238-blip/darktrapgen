#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class MidiDragBox final : public juce::Component
{
public:
    explicit MidiDragBox (DarkTrapAudioProcessor& p) : processorRef (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        g.setColour (juce::Colour (0xff1d1d1d));
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (juce::Colour (0xff9b59b6));
        g.drawRoundedRectangle (bounds, 8.0f, 2.0f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (16.0f, juce::Font::bold));
        g.drawText ("DRAG MIDI TO DAW",
                    getLocalBounds().removeFromTop (getHeight() / 2),
                    juce::Justification::centred);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (12.0f);
        g.drawText ("Click + drag onto FL Studio piano roll",
                    getLocalBounds().removeFromBottom (getHeight() / 2),
                    juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        dragging = false;
    }

    void mouseDrag (const juce::MouseEvent&) override
    {
        if (dragging)
            return;

        dragging = true;

        const auto midiFile = processorRef.getGeneratedMidiFile();

        if (midiFile.existsAsFile())
        {
            juce::StringArray files;
            files.add (midiFile.getFullPathName());

            juce::DragAndDropContainer::performExternalDragDropOfFiles (
                files, true, this);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragging = false;
    }

private:
    DarkTrapAudioProcessor& processorRef;
    bool dragging = false;
};

//==============================================================================
class DarkTrapAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           public juce::DragAndDropContainer
{
public:
    explicit DarkTrapAudioProcessorEditor (DarkTrapAudioProcessor&);
    ~DarkTrapAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DarkTrapAudioProcessor& processorRef;

    juce::TextButton generateButton { "GENERATE" };
    juce::ComboBox presetBox;
    juce::TextButton loadSoundButton { "LOAD SF2 / SFZ / WAV" };
    juce::ToggleButton halfTimeButton { "Half-Time" };

    MidiDragBox midiDragBox;

    juce::Slider cutoffSlider, attackSlider, decaySlider, sustainSlider,
                 releaseSlider, reverbSlider, delaySlider, wobbleSlider;

    juce::Label cutoffLabel, attackLabel, decayLabel, sustainLabel,
                releaseLabel, reverbLabel, delayLabel, wobbleLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> halfTimeAttachment;

    void configureSlider (juce::Slider& slider,
                          juce::Label& label,
                          const juce::String& paramID,
                          const juce::String& labelText);

    void positionSlider (juce::Slider& slider,
                         juce::Label& label,
                         int x, int y, int w, int h);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTrapAudioProcessorEditor)
};