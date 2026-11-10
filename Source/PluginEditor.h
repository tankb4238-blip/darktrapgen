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
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    DarkTrapAudioProcessor& processorRef;
    bool dragging = false;
};

//==============================================================================
class XYExperimentPad final : public juce::Component
{
public:
    explicit XYExperimentPad (DarkTrapAudioProcessor& p) : processorRef (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { update (e.position); }
    void mouseDrag (const juce::MouseEvent& e) override { update (e.position); }

private:
    void update (juce::Point<float>);
    DarkTrapAudioProcessor& processorRef;
    float thumbX = 0.7f, thumbY = 0.5f;
};

//==============================================================================
class PianoRollViewer final : public juce::Component
{
public:
    explicit PianoRollViewer (DarkTrapAudioProcessor& p) : processorRef (p) {}
    void paint (juce::Graphics&) override;

private:
    DarkTrapAudioProcessor& processorRef;
};

//==============================================================================
class LayerRow final : public juce::Component
{
public:
    LayerRow (DarkTrapAudioProcessor& p, int layerIndex);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void openFileLoader();

    DarkTrapAudioProcessor& processorRef;
    int index;
    juce::ComboBox presetBox;
    juce::TextButton loadFileButton { "LOAD SF2/WAV" };
    juce::Label nameLabel;
    juce::Slider volumeSlider;
    juce::TextButton muteButton { "M" };
    std::unique_ptr<juce::FileChooser> fileChooser;
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
    void timerCallback() override;

private:
    DarkTrapAudioProcessor& processorRef;

    juce::TextButton generateButton { "GENERATE" };
    juce::ComboBox keyBox, moodBox, genreBox;
    juce::TextButton exportAudioButton { "EXPORT .WAV" };
    juce::TextButton exportStemsButton { "EXPORT STEMS" };

    std::array<std::unique_ptr<LayerRow>, 3> layerRows;
    std::array<juce::TextButton, 8> variationButtons;

    XYExperimentPad xyPad { processorRef };
    PianoRollViewer pianoRoll { processorRef };
    MidiDragBox midiDragBox { processorRef };

    juce::Slider cutoffKnob, reverbKnob, delayKnob, chorusKnob,
                 distortionKnob, flangerKnob, attackKnob, releaseKnob;
    juce::ToggleButton halfSpeedButton { "Half-Speed" };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> halfSpeedAttachment;

    std::unique_ptr<juce::FileChooser> audioChooser, stemsChooser;

    void configureKnob (juce::Slider& s, const juce::String& id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTrapAudioProcessorEditor)
};
