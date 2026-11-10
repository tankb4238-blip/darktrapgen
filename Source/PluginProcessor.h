#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>
#include <array>

//==============================================================================
struct VoiceParams
{
    std::atomic<float>* cutoff  = nullptr;
    std::atomic<float>* attack  = nullptr;
    std::atomic<float>* decay   = nullptr;
    std::atomic<float>* sustain = nullptr;
    std::atomic<float>* release = nullptr;
};

//==============================================================================
class UniversalSound final : public juce::SynthesiserSound
{
public:
    explicit UniversalSound (const juce::String& nameIn = "Sound") : soundName (nameIn) {}
    bool appliesToNote (int n) override { return n >= lowNote && n <= highNote; }
    bool appliesToChannel (int) override { return true; }

    juce::String soundName;
    juce::AudioBuffer<float> sampleBuffer;
    double sourceSampleRate = 44100.0;
    int rootNote = 60;
    int lowNote = 0, highNote = 127;
    int loopStart = 0, loopEnd = 0;
    bool loopEnabled = false;
    bool isModal = false;
    float gain = 1.0f;
    float tuneCents = 0.0f;

    int numPartials = 0;
    double partialRatios[8] {};
    double partialAmps[8] {};
    double partialDecays[8] {};
    float noiseAmount = 0.0f;
    float driveAmount = 0.0f;

    bool useCustomEnvelope = false;
    float presetAttack  = 0.01f;
    float presetDecay   = 0.20f;
    float presetSustain = 0.80f;
    float presetRelease = 0.30f;
    float presetCutoff  = 8000.0f;
};

//==============================================================================
class UniversalVoice final : public juce::SynthesiserVoice
{
public:
    explicit UniversalVoice (VoiceParams& paramsIn);

    bool canPlaySound (juce::SynthesiserSound* s) override
    { return dynamic_cast<UniversalSound*> (s) != nullptr; }

    void startNote (int, float, juce::SynthesiserSound*, int) override;
    void stopNote (float, bool) override;
    void pitchWheelMoved (int) override;
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int, int) override;

private:
    VoiceParams& params;
    UniversalSound* currentSound = nullptr;
    juce::ADSR adsr;
    juce::IIRFilter filterL, filterR;
    double samplePos = 0.0, pitchRatio = 1.0, lastSampleRate = 0.0;
    float velocityGain = 1.0f, pitchWheelSemitones = 0.0f;
    int midiNote = 60;
    bool modalActive = false;
    double modalTime = 0.0;
    static constexpr int maxModes = 8;
    int numActiveModes = 6;
    double modalFreq[maxModes] {}, modalPhase[maxModes] {},
           modalAmp[maxModes] {}, modalDecay[maxModes] {};

    void initModal (int noteNumber);
    void updateFilters (double sr);
    float readSample (int channel, double position) const;
};

//==============================================================================
struct LayerSource
{
    juce::Synthesiser synth;
    float volume = 0.8f;
    bool muted = false;
    juce::String sourceName = "Init";
    int presetIndex = 0;
};

//==============================================================================
struct GeneratedVariation
{
    juce::MidiMessageSequence melodySeq;
    juce::MidiMessageSequence chordSeq;
    juce::MidiMessageSequence bassSeq;
    juce::String moodTag, genreTag;
    double bpm = 140.0;
    int rootNote = 48;
    int bars = 4;
};

//==============================================================================
struct DarkInstrumentDef
{
    const char* name;
    const char* category;
    int numPartials;
    double ratios[8];
    double amps[8];
    double decays[8];
    float noiseAmount;
    float driveAmount;
    int lowNote, highNote;
    float attack, decay, sustain, release, cutoff;
};

//==============================================================================
class DarkTrapAudioProcessor final : public juce::AudioProcessor
{
public:
    DarkTrapAudioProcessor();
    ~DarkTrapAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Dark Trap Melody Generator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    void generateVariations();
    void setActiveVariation (int index);
    int  getActiveVariation() const { return currentVariationIndex; }
    int  getNumVariations() const { return (int) variations.size(); }

    juce::File getVariationMidiFile (int index);

    void loadDarkInstrumentPreset (int layerIndex, int presetIndex);
    void loadSoundIntoLayer (int layerIndex, const juce::Array<juce::File>& files);

    void exportAudioToFile (const juce::File& outputFile);
    void exportLayerStems (const juce::File& folder);

    void setXYPad (float x, float y);

    std::atomic<int> selectedKey   { 0 };
    std::atomic<int> selectedMood  { 0 };
    std::atomic<int> selectedGenre { 0 };

    std::atomic<float> layer0Vol { 1.0f };
    std::atomic<float> layer1Vol { 1.0f };
    std::atomic<float> layer2Vol { 1.0f };

    LayerSource& getLayer (int index) { return layers[(size_t) juce::jlimit (0, 2, index)]; }
    const std::vector<GeneratedVariation>& getVariations() const { return variations; }

    static int getNumDarkInstruments();
    static const DarkInstrumentDef& getDarkInstrument (int index);

    juce::AudioProcessorValueTreeState apvts;

private:
    std::array<LayerSource, 3> layers;
    juce::CriticalSection synthLock;
    VoiceParams voiceParams;
    juce::AudioFormatManager formatManager;
    juce::Reverb reverb;

    juce::AudioBuffer<float> delayBuffer;      int delayWritePos = 0;
    juce::AudioBuffer<float> chorusBuffer;     int chorusWritePos = 0; float chorusPhase = 0.0f;
    juce::AudioBuffer<float> flangerBuffer;    int flangerWritePos = 0; float flangerPhase = 0.0f;
    juce::AudioBuffer<float> halfSpeedBuffer;  int halfSpeedWritePos = 0; double halfSpeedReadPos = 0.0;

    std::vector<GeneratedVariation> variations;
    int currentVariationIndex = 0;
    double generatedPlaybackTime = 0.0;
    bool auditioning = false;
    int generatedTicksPerQuarterNote = 960;
    double generatedLengthSeconds = 0.0;

    void setupLayer (int index);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTrapAudioProcessor)
};
