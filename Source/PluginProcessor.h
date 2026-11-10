#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

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
    explicit UniversalSound (const juce::String& nameIn = "Sound")
        : soundName (nameIn)
    {
    }

    bool appliesToNote (int midiNoteNumber) override
    {
        return midiNoteNumber >= lowNote && midiNoteNumber <= highNote;
    }

    bool appliesToChannel (int) override { return true; }

    juce::String soundName;
    juce::AudioBuffer<float> sampleBuffer;
    double sourceSampleRate = 44100.0;
    int rootNote = 60;
    int lowNote = 0;
    int highNote = 127;
    int loopStart = 0;
    int loopEnd = 0;
    bool loopEnabled = false;
    bool isModal = false;
    float gain = 1.0f;
    float tuneCents = 0.0f;
};

//==============================================================================
class UniversalVoice final : public juce::SynthesiserVoice
{
public:
    explicit UniversalVoice (VoiceParams& paramsIn)
        : params (paramsIn)
    {
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<UniversalSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newValue) override;
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override;

private:
    VoiceParams& params;
    UniversalSound* currentSound = nullptr;

    juce::ADSR adsr;
    juce::IIRFilter filterL;
    juce::IIRFilter filterR;

    double samplePos = 0.0;
    double pitchRatio = 1.0;
    double lastSampleRate = 0.0;
    float velocityGain = 1.0f;
    int midiNote = 60;
    float pitchWheelSemitones = 0.0f;

    bool modalActive = false;
    double modalTime = 0.0;
    static constexpr int numModes = 6;
    double modalFreq[numModes] {};
    double modalPhase[numModes] {};
    double modalAmp[numModes] {};
    double modalDecay[numModes] {};

    void initModal (int noteNumber);
    void updateFilters (double sr);
    float readSample (int channel, double position) const;
};

//==============================================================================
class DarkTrapAudioProcessor final : public juce::AudioProcessor
{
public:
    DarkTrapAudioProcessor();
    ~DarkTrapAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark Trap Melody Generator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    void generateMelody();
    juce::File getGeneratedMidiFile();
    void loadInstrumentBank (const juce::Array<juce::File>& files);
    void loadPreset (int index);

    juce::AudioProcessorValueTreeState apvts;

private:
    struct TimedEvent
    {
        double timeSeconds = 0.0;
        juce::MidiMessage message;
    };

    juce::Synthesiser synth;
    juce::CriticalSection synthLock;
    VoiceParams voiceParams;
    juce::AudioFormatManager formatManager;
    juce::Reverb reverb;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    juce::AudioBuffer<float> wobbleBuffer;
    int wobbleWritePos = 0;
    float wobblePhase = 0.0f;

    juce::MidiMessageSequence generatedSequence;
    std::vector<TimedEvent> generatedEvents;
    double generatedLengthSeconds = 0.0;
    double generatedPlaybackTime = 0.0;
    bool auditioning = false;
    int generatedTicksPerQuarterNote = 960;
    double generatedTempoBpm = 140.0;

    void resetSounds();
    void createDefaultModalSound();
    void loadSf2 (const juce::File& file);
    void loadSfz (const juce::File& file);
    void loadWavFiles (const juce::Array<juce::File>& files);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTrapAudioProcessor)
};