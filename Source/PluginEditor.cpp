#include "PluginEditor.h"

//==============================================================================
void MidiDragBox::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff1a0a1a)); g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colour (0xff9b59b6)); g.drawRoundedRectangle (b, 8.0f, 2.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
    g.drawText ("DRAG MIDI TO DAW", getLocalBounds().removeFromTop (getHeight() / 2),
                juce::Justification::centred);
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("3 tracks: melody / chords / bass",
                getLocalBounds().removeFromBottom (getHeight() / 2),
                juce::Justification::centred);
}

void MidiDragBox::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    const auto midiFile = processorRef.getVariationMidiFile (processorRef.getActiveVariation());
    if (midiFile.existsAsFile())
    {
        juce::StringArray files;
        files.add (midiFile.getFullPathName());
        juce::DragAndDropContainer::performExternalDragDropOfFiles (files, true, this);
    }
}

//==============================================================================
void XYExperimentPad::update (juce::Point<float> pos)
{
    thumbX = juce::jlimit (0.0f, 1.0f, pos.x / (float) getWidth());
    thumbY = juce::jlimit (0.0f, 1.0f, pos.y / (float) getHeight());
    processorRef.setXYPad (thumbX, thumbY);
    repaint();
}

void XYExperimentPad::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    juce::ColourGradient grad (juce::Colour (0xff0a0512), 0, 0,
                                juce::Colour (0xff1a0a2e), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (grad); g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colour (0x30ffffff));
    for (int i = 1; i < 4; ++i)
    {
        const float x = b.getX() + b.getWidth() * (float) i / 4.0f;
        g.drawLine (x, b.getY(), x, b.getBottom(), 1.0f);
        const float y = b.getY() + b.getHeight() * (float) i / 4.0f;
        g.drawLine (b.getX(), y, b.getRight(), y, 1.0f);
    }
    const auto thumb = juce::Point<float> (b.getX() + thumbX * b.getWidth(),
                                            b.getY() + thumbY * b.getHeight());
    g.setColour (juce::Colour (0xffe74c3c));
    g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (thumb));
    g.setColour (juce::Colours::white);
    g.drawEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (thumb), 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    auto t = getLocalBounds().reduced (6);
    g.drawText ("CUTOFF \xe2\x86\x92", t.removeFromTop (14), juce::Justification::centredRight);
    g.drawText ("LAYER BLEND", getLocalBounds().reduced (6), juce::Justification::bottomLeft);
}

//==============================================================================
void PianoRollViewer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0e0a0e)); g.fillRoundedRectangle (b, 6.0f);

    const auto& vars = processorRef.getVariations();
    if (vars.empty())
    {
        g.setColour (juce::Colours::grey);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText ("Press GENERATE", b, juce::Justification::centred);
        return;
    }
    const auto& v = vars[(size_t) juce::jlimit (0, (int) vars.size() - 1,
                                                 processorRef.getActiveVariation())];
    int lo = 127, hi = 0;
    auto scan = [&] (const juce::MidiMessageSequence& s) {
        for (int i = 0; i < s.getNumEvents(); ++i)
        {
            const auto* ev = s.getEventPointer (i);
            if (ev && ev->message.isNoteOn())
            {
                int n = ev->message.getNoteNumber();
                if (n < lo) lo = n; if (n > hi) hi = n;
            }
        }
    };
    scan (v.melodySeq); scan (v.chordSeq); scan (v.bassSeq);
    if (hi <= lo) { lo = 24; hi = 96; }
    lo = juce::jmax (12, lo - 2); hi = juce::jmin (120, hi + 2);

    const float rowH = b.getHeight() / (float)(hi - lo + 1);
    const float total = (float)(v.bars * 4 * 960);

    g.setColour (juce::Colour (0x20ffffff));
    for (int bar = 1; bar < v.bars * 4; ++bar)
    {
        const float x = b.getX() + b.getWidth() * (float) bar / (float)(v.bars * 4);
        g.drawLine (x, b.getY(), x, b.getBottom(), 1.0f);
    }

    auto draw = [&] (const juce::MidiMessageSequence& s, juce::Colour c) {
        g.setColour (c);
        for (int i = 0; i < s.getNumEvents(); ++i)
        {
            const auto* ev = s.getEventPointer (i);
            if (! ev || ! ev->message.isNoteOn()) continue;
            const int n = ev->message.getNoteNumber();
            const float x = b.getX() + b.getWidth() * (float)(s.getEventTime (i) / total);
            const float y = b.getY() + b.getHeight() * (1.0f - (float)(n - lo) / (float)(hi - lo + 1));
            g.fillRoundedRectangle (x, y, 4.0f, juce::jmax (2.0f, rowH - 1.0f), 1.0f);
        }
    };
    draw (v.bassSeq,   juce::Colour (0xff3498db));
    draw (v.chordSeq,  juce::Colour (0xffe67e22));
    draw (v.melodySeq, juce::Colour (0xffe74c3c));
}

//==============================================================================
LayerRow::LayerRow (DarkTrapAudioProcessor& p, int i) : processorRef (p), index (i)
{
    addAndMakeVisible (presetBox);
    const int n = DarkTrapAudioProcessor::getNumDarkInstruments();
    for (int k = 0; k < n; ++k)
    {
        const auto& d = DarkTrapAudioProcessor::getDarkInstrument (k);
        presetBox.addItem (juce::String (d.name) + "  [" + d.category + "]", k + 1);
    }
    presetBox.setSelectedId (processorRef.getLayer (index).presetIndex + 1, juce::dontSendNotification);
    presetBox.onChange = [this] {
        const int sel = presetBox.getSelectedId() - 1;
        if (sel >= 0) processorRef.loadDarkInstrumentPreset (index, sel);
        nameLabel.setText (processorRef.getLayer (index).sourceName, juce::dontSendNotification);
    };

    addAndMakeVisible (loadFileButton);
    loadFileButton.onClick = [this] { openFileLoader(); };

    addAndMakeVisible (nameLabel);
    nameLabel.setText ("L" + juce::String (index + 1) + ": " + processorRef.getLayer (index).sourceName,
                       juce::dontSendNotification);
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    nameLabel.setFont (juce::Font (juce::FontOptions (11.0f)));

    addAndMakeVisible (volumeSlider);
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange (0.0, 1.0, 0.01);
    volumeSlider.setValue (0.8);
    volumeSlider.onValueChange = [this] {
        processorRef.getLayer (index).volume = (float) volumeSlider.getValue();
    };

    addAndMakeVisible (muteButton);
    muteButton.setClickingTogglesState (true);
    muteButton.onClick = [this] { processorRef.getLayer (index).muted = muteButton.getToggleState(); };
}

void LayerRow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a0a1a));
    g.fillRoundedRectangle (b.reduced (2.0f), 5.0f);
    g.setColour (juce::Colour (0xff3a1a3a));
    g.drawRoundedRectangle (b.reduced (2.0f), 5.0f, 1.0f);
}

void LayerRow::resized()
{
    auto area = getLocalBounds().reduced (6);
    auto top = area.removeFromTop (20);
    nameLabel.setBounds (top.removeFromLeft (150));
    muteButton.setBounds (top.removeFromRight (24));
    auto row2 = area.removeFromTop (24);
    presetBox.setBounds (row2.removeFromLeft (getWidth() - 100));
    row2.removeFromLeft (4);
    loadFileButton.setBounds (row2);
    volumeSlider.setBounds (area.reduced (0, 4));
}

void LayerRow::openFileLoader()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load SF2 / SFZ / WAV into layer " + juce::String (index + 1),
        juce::File(), "*.sf2;*.sfz;*.wav");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::canSelectMultipleItems,
        [this] (const juce::FileChooser& fc)
        {
            const auto res = fc.getResults();
            if (! res.isEmpty())
            {
                processorRef.loadSoundIntoLayer (index, res);
                nameLabel.setText (processorRef.getLayer (index).sourceName, juce::dontSendNotification);
                presetBox.setSelectedId (0, juce::dontSendNotification);
            }
        });
}

//==============================================================================
DarkTrapAudioProcessorEditor::DarkTrapAudioProcessorEditor (DarkTrapAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (generateButton);
    generateButton.onClick = [this] { processorRef.generateVariations(); };

    addAndMakeVisible (keyBox);
    keyBox.addItemList ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    keyBox.setSelectedId (1, juce::dontSendNotification);
    keyBox.onChange = [this] { processorRef.selectedKey.store (keyBox.getSelectedItemIndex()); };

    addAndMakeVisible (moodBox);
    moodBox.addItemList ({ "Dark", "Haunting", "Sinister", "Eerie" }, 1);
    moodBox.setSelectedId (1, juce::dontSendNotification);
    moodBox.onChange = [this] { processorRef.selectedMood.store (moodBox.getSelectedItemIndex()); };

    addAndMakeVisible (genreBox);
    genreBox.addItemList ({ "Trap", "Drill", "Plugg", "Ambient" }, 1);
    genreBox.setSelectedId (1, juce::dontSendNotification);
    genreBox.onChange = [this] { processorRef.selectedGenre.store (genreBox.getSelectedItemIndex()); };

    addAndMakeVisible (exportAudioButton);
    exportAudioButton.onClick = [this] {
        audioChooser = std::make_unique<juce::FileChooser> ("Export audio",
            juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("DarkTrap.wav"), "*.wav");
        audioChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc) {
                const auto f = fc.getResult();
                if (f != juce::File()) processorRef.exportAudioToFile (f.withFileExtension (".wav"));
            });
    };

    addAndMakeVisible (exportStemsButton);
    exportStemsButton.onClick = [this] {
        stemsChooser = std::make_unique<juce::FileChooser> ("Export stems folder",
            juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("DarkTrap_Stems"));
        stemsChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& fc) {
                const auto f = fc.getResult();
                if (f != juce::File()) { f.createDirectory(); processorRef.exportLayerStems (f); }
            });
    };

    for (int i = 0; i < 3; ++i) { layerRows[(size_t) i] = std::make_unique<LayerRow> (processorRef, i); addAndMakeVisible (*layerRows[(size_t) i]); }

    for (int v = 0; v < 8; ++v)
    {
        variationButtons[(size_t) v].setButtonText (juce::String (v + 1));
        variationButtons[(size_t) v].onClick = [this, v] { processorRef.setActiveVariation (v); };
        addAndMakeVisible (variationButtons[(size_t) v]);
    }

    addAndMakeVisible (xyPad);
    addAndMakeVisible (pianoRoll);
    addAndMakeVisible (midiDragBox);

    configureKnob (cutoffKnob,     "cutoff");
    configureKnob (reverbKnob,     "reverb");
    configureKnob (delayKnob,      "delay");
    configureKnob (chorusKnob,     "chorus");
    configureKnob (distortionKnob, "distortion");
    configureKnob (flangerKnob,    "flanger");
    configureKnob (attackKnob,     "attack");
    configureKnob (releaseKnob,    "release");

    addAndMakeVisible (halfSpeedButton);
    halfSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, "halfSpeed", halfSpeedButton);

    setSize (1100, 700);
    startTimerHz (30);
}

void DarkTrapAudioProcessorEditor::configureKnob (juce::Slider& s, const juce::String& id)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.15f,
                            juce::MathConstants<float>::pi * 2.85f, true);
    addAndMakeVisible (s);
    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, id, s));
}

void DarkTrapAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (juce::Colour (0xff0a0512), 0, 0,
                                juce::Colour (0xff1a0a1a), 0, (float) getHeight(), false);
    g.setGradientFill (grad); g.fillAll();
}

void DarkTrapAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto top = area.removeFromTop (44);
    generateButton.setBounds (top.removeFromLeft (120));
    top.removeFromLeft (8);
    keyBox.setBounds (top.removeFromLeft (70));
    top.removeFromLeft (4);
    moodBox.setBounds (top.removeFromLeft (120));
    top.removeFromLeft (4);
    genreBox.setBounds (top.removeFromLeft (110));
    top.removeFromLeft (12);
    exportAudioButton.setBounds (top.removeFromLeft (120));
    top.removeFromLeft (4);
    exportStemsButton.setBounds (top.removeFromLeft (120));

    area.removeFromTop (8);

    auto mid = area.removeFromTop (320);

    auto layersCol = mid.removeFromLeft (380);
    for (int i = 0; i < 3; ++i) layerRows[(size_t) i]->setBounds (layersCol.removeFromTop (100).reduced (0, 2));

    mid.removeFromLeft (8);
    auto rightCol = mid;

    auto padArea = rightCol.removeFromRight (220);
    xyPad.setBounds (padArea.removeFromTop (220));
    padArea.removeFromTop (8);
    midiDragBox.setBounds (padArea.removeFromTop (90));
    pianoRoll.setBounds (rightCol);

    area.removeFromTop (8);

    auto bottom = area.removeFromTop (100);
    auto variationsArea = bottom.removeFromLeft (480);
    for (int v = 0; v < 8; ++v) variationButtons[(size_t) v].setBounds (variationsArea.removeFromLeft (58).reduced (2));

    bottom.removeFromLeft (12);
    auto fxArea = bottom;

    const int knobW = 72;
    cutoffKnob.setBounds     (fxArea.removeFromLeft (knobW));
    reverbKnob.setBounds     (fxArea.removeFromLeft (knobW));
    delayKnob.setBounds      (fxArea.removeFromLeft (knobW));
    chorusKnob.setBounds     (fxArea.removeFromLeft (knobW));
    distortionKnob.setBounds (fxArea.removeFromLeft (knobW));
    flangerKnob.setBounds    (fxArea.removeFromLeft (knobW));
    attackKnob.setBounds     (fxArea.removeFromLeft (knobW));
    releaseKnob.setBounds    (fxArea.removeFromLeft (knobW));
    halfSpeedButton.setBounds (fxArea.removeFromLeft (100).reduced (0, 40));
}

void DarkTrapAudioProcessorEditor::timerCallback()
{
    pianoRoll.repaint();
}
