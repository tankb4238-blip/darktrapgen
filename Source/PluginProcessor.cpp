#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>
#include <cmath>
#include <map>
#include <algorithm>

//==============================================================================
// Small helpers for the built-in SF2 / SFZ readers.
//==============================================================================
namespace
{
    constexpr int GEN_KEY_RANGE           = 43;
    constexpr int GEN_VEL_RANGE           = 44;
    constexpr int GEN_INSTRUMENT          = 41;
    constexpr int GEN_SAMPLE_ID           = 53;
    constexpr int GEN_OVERRIDING_ROOT_KEY = 58;
    constexpr int GEN_SAMPLE_MODES        = 54;
    constexpr int GEN_COARSE_TUNE         = 51;
    constexpr int GEN_FINE_TUNE           = 52;
    constexpr int GEN_INITIAL_ATTENUATION = 48;

    inline juce::uint16 readU16 (const juce::uint8* p)
    {
        return (juce::uint16) (p[0] | (p[1] << 8));
    }

    inline juce::uint32 readU32 (const juce::uint8* p)
    {
        return (juce::uint32) p[0]
             | ((juce::uint32) p[1] << 8)
             | ((juce::uint32) p[2] << 16)
             | ((juce::uint32) p[3] << 24);
    }

    inline juce::int16 readI16 (const juce::uint8* p)
    {
        return (juce::int16) readU16 (p);
    }

    struct RiffChunk
    {
        const juce::uint8* data = nullptr;
        juce::uint32 size = 0;
    };

    bool parseSf2Chunks (const juce::uint8* data,
                         size_t size,
                         std::map<juce::String, RiffChunk>& chunks)
    {
        if (size < 12
            || std::memcmp (data, "RIFF", 4) != 0
            || std::memcmp (data + 8, "sfbk", 4) != 0)
            return false;

        size_t pos = 12;

        while (pos + 8 <= size)
        {
            const juce::uint32 chunkSize = readU32 (data + pos + 4);

            if (pos + 8 + chunkSize > size)
                break;

            const juce::String id ((const char*) (data + pos), 4);

            if (id == "LIST")
            {
                const juce::String listType ((const char*) (data + pos + 8), 4);

                if (listType == "sdta" || listType == "pdta")
                {
                    size_t subPos = pos + 12;
                    const size_t listEnd = pos + 8 + chunkSize;

                    while (subPos + 8 <= listEnd)
                    {
                        const juce::uint32 subSize = readU32 (data + subPos + 4);

                        if (subPos + 8 + subSize > listEnd)
                            break;

                        const juce::String subId ((const char*) (data + subPos), 4);
                        chunks[subId] = { data + subPos + 8, subSize };

                        subPos += 8 + subSize + (subSize & 1);
                    }
                }
            }

            pos += 8 + chunkSize + (chunkSize & 1);
        }

        return true;
    }

    int guessRootNoteFromName (const juce::String& name)
    {
        const auto upper = name.toUpperCase();

        for (int i = 0; i < upper.length(); ++i)
        {
            const auto c = upper[i];
            int semitone = -1;

            if (c == 'C') semitone = 0;
            else if (c == 'D') semitone = 2;
            else if (c == 'E') semitone = 4;
            else if (c == 'F') semitone = 5;
            else if (c == 'G') semitone = 7;
            else if (c == 'A') semitone = 9;
            else if (c == 'B') semitone = 11;

            if (semitone >= 0)
            {
                int idx = i + 1;

                if (idx < upper.length() && (upper[idx] == '#' || upper[idx] == 'S'))
                {
                    ++semitone;
                    ++idx;
                }
                else if (idx < upper.length() && upper[idx] == 'B')
                {
                    --semitone;
                    ++idx;
                }

                if (idx < upper.length() && juce::CharacterFunctions::isDigit (upper[idx]))
                {
                    const int octave = upper[idx] - '0';
                    return juce::jlimit (0, 127, 12 * (octave + 1) + semitone);
                }
            }
        }

        const auto digits = upper.retainCharacters ("0123456789");

        if (digits.isNotEmpty())
        {
            const int value = digits.getIntValue();

            if (value >= 0 && value <= 127)
                return value;
        }

        return 60;
    }

    bool loadAudioFileToBuffer (juce::AudioFormatManager& formatManager,
                                const juce::File& file,
                                juce::AudioBuffer<float>& buffer,
                                double& sampleRate)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

        if (reader == nullptr || reader->lengthInSamples <= 0)
            return false;

        const int numChannels = juce::jlimit (1, 2, (int) reader->numChannels);
        const int numSamples = (int) juce::jmin (reader->lengthInSamples,
                                                 (juce::int64) std::numeric_limits<int>::max());

        buffer.setSize (numChannels, numSamples);
        reader->read (&buffer, 0, numSamples, 0, true, true);

        sampleRate = reader->sampleRate;
        return true;
    }

    bool loadSf2Into (const juce::File& file, juce::Synthesiser& synth)
    {
        juce::MemoryBlock memory;
        file.loadFileAsData (memory);

        const auto* data = static_cast<const juce::uint8*> (memory.getData());
        const auto size = memory.getSize();

        std::map<juce::String, RiffChunk> chunks;

        if (! parseSf2Chunks (data, size, chunks))
            return false;

        const auto smplIt = chunks.find ("smpl");
        const auto phdrIt = chunks.find ("phdr");
        const auto pbagIt = chunks.find ("pbag");
        const auto pgenIt = chunks.find ("pgen");
        const auto instIt = chunks.find ("inst");
        const auto ibagIt = chunks.find ("ibag");
        const auto igenIt = chunks.find ("igen");
        const auto shdrIt = chunks.find ("shdr");

        if (smplIt == chunks.end()
            || phdrIt == chunks.end()
            || pbagIt == chunks.end()
            || pgenIt == chunks.end()
            || instIt == chunks.end()
            || ibagIt == chunks.end()
            || igenIt == chunks.end()
            || shdrIt == chunks.end())
            return false;

        const auto* smpl = smplIt->second.data;
        const auto* phdr = phdrIt->second.data;
        const auto* pbag = pbagIt->second.data;
        const auto* pgen = pgenIt->second.data;
        const auto* inst = instIt->second.data;
        const auto* ibag = ibagIt->second.data;
        const auto* igen = igenIt->second.data;
        const auto* shdr = shdrIt->second.data;

        const int smplFrames = (int) (smplIt->second.size / 2);
        const int phdrCount = (int) (phdrIt->second.size / 38);
        const int pbagCount = (int) (pbagIt->second.size / 4);
        const int pgenCount = (int) (pgenIt->second.size / 4);
        const int instCount = (int) (instIt->second.size / 22);
        const int ibagCount = (int) (ibagIt->second.size / 4);
        const int igenCount = (int) (igenIt->second.size / 4);
        const int shdrCount = (int) (shdrIt->second.size / 46);

        if (phdrCount < 2 || pbagCount < 1 || pgenCount < 1
            || instCount < 2 || ibagCount < 1 || igenCount < 1
            || shdrCount < 2 || smplFrames <= 0)
            return false;

        bool addedAny = false;

        // Load every preset zone we can. This is intentionally permissive:
        // it will make sound for most GM / trap SF2 files without needing
        // external FluidSynth / TinySoundFont dependencies.
        for (int pi = 0; pi + 1 < phdrCount; ++pi)
        {
            const auto* presetRec = phdr + pi * 38;
            const auto* nextPresetRec = phdr + (pi + 1) * 38;

            const int pbagStart = readU16 (presetRec + 24);
            const int pbagEnd   = readU16 (nextPresetRec + 24);

            for (int pbi = pbagStart; pbi < pbagEnd && pbi < pbagCount - 1; ++pbi)
            {
                const int genStart = readU16 (pbag + pbi * 4);
                const int genEnd   = readU16 (pbag + (pbi + 1) * 4);

                int instrumentIndex = -1;

                for (int gi = genStart; gi < genEnd && gi < pgenCount - 1; ++gi)
                {
                    const int oper = readU16 (pgen + gi * 4);

                    if (oper == GEN_INSTRUMENT)
                    {
                        instrumentIndex = readU16 (pgen + gi * 4 + 2);
                        break;
                    }
                }

                if (instrumentIndex < 0 || instrumentIndex + 1 >= instCount)
                    continue;

                const auto* instrumentRec = inst + instrumentIndex * 22;
                const auto* nextInstrumentRec = inst + (instrumentIndex + 1) * 22;

                const int ibagStart = readU16 (instrumentRec + 20);
                const int ibagEnd   = readU16 (nextInstrumentRec + 20);

                for (int ibi = ibagStart; ibi < ibagEnd && ibi < ibagCount - 1; ++ibi)
                {
                    const int igStart = readU16 (ibag + ibi * 4);
                    const int igEnd   = readU16 (ibag + (ibi + 1) * 4);

                    int sampleIndex = -1;
                    int rootKey = -1;
                    int keyLo = 0, keyHi = 127;
                    int velLo = 0, velHi = 127;
                    int sampleModes = 0;
                    int coarseTune = 0;
                    int fineTune = 0;
                    int attenuation = 0;

                    for (int gi = igStart; gi < igEnd && gi < igenCount - 1; ++gi)
                    {
                        const int oper = readU16 (igen + gi * 4);
                        const int amount = readU16 (igen + gi * 4 + 2);

                        switch (oper)
                        {
                            case GEN_SAMPLE_ID:
                                sampleIndex = amount;
                                break;
                            case GEN_KEY_RANGE:
                                keyLo = amount & 0xFF;
                                keyHi = (amount >> 8) & 0xFF;
                                break;
                            case GEN_VEL_RANGE:
                                velLo = amount & 0xFF;
                                velHi = (amount >> 8) & 0xFF;
                                break;
                            case GEN_OVERRIDING_ROOT_KEY:
                                rootKey = amount;
                                break;
                            case GEN_SAMPLE_MODES:
                                sampleModes = amount;
                                break;
                            case GEN_COARSE_TUNE:
                                coarseTune = (juce::int16) amount;
                                break;
                            case GEN_FINE_TUNE:
                                fineTune = (juce::int16) amount;
                                break;
                            case GEN_INITIAL_ATTENUATION:
                                attenuation = amount;
                                break;
                            default:
                                break;
                        }
                    }

                    juce::ignoreUnused (velLo, velHi);

                    if (sampleIndex < 0 || sampleIndex + 1 >= shdrCount)
                        continue;

                    const auto* sh = shdr + sampleIndex * 46;

                    const auto sampleStart = readU32 (sh + 20);
                    const auto sampleEnd   = readU32 (sh + 24);
                    const auto loopStartAbs = readU32 (sh + 28);
                    const auto loopEndAbs   = readU32 (sh + 32);
                    const auto sampleRate  = readU32 (sh + 36);
                    const auto originalPitch = (int) sh[40];
                    const auto pitchCorrection = (int) (juce::int8) sh[41];

                    if (sampleEnd <= sampleStart || sampleEnd > (juce::uint32) smplFrames)
                        continue;

                    const int numFrames = (int) (sampleEnd - sampleStart);

                    if (numFrames <= 0)
                        continue;

                    auto* sound = new UniversalSound ("SF2");
                    sound->sampleBuffer.setSize (1, numFrames);

                    for (int i = 0; i < numFrames; ++i)
                    {
                        const auto s = readI16 (smpl + (sampleStart + (juce::uint32) i) * 2);
                        sound->sampleBuffer.setSample (0, i, (float) s / 32768.0f);
                    }

                    sound->sourceSampleRate = sampleRate > 0 ? (double) sampleRate : 44100.0;
                    sound->rootNote = (rootKey >= 0 && rootKey <= 127)
                                        ? rootKey
                                        : juce::jlimit (0, 127, originalPitch);

                    sound->lowNote = juce::jlimit (0, 127, keyLo);
                    sound->highNote = juce::jlimit (0, 127, keyHi);

                    if (sound->lowNote > sound->highNote)
                        std::swap (sound->lowNote, sound->highNote);

                    sound->loopEnabled = (sampleModes & 1) != 0 || sampleModes == 3;

                    if (sound->loopEnabled)
                    {
                        const int ls = (int) (loopStartAbs - sampleStart);
                        const int le = (int) (loopEndAbs - sampleStart);

                        if (ls >= 0 && le > ls && le <= numFrames)
                        {
                            sound->loopStart = ls;
                            sound->loopEnd = le;
                        }
                        else
                        {
                            sound->loopStart = 0;
                            sound->loopEnd = numFrames;
                        }
                    }

                    sound->tuneCents = (float) (pitchCorrection + fineTune + coarseTune * 100);
                    sound->gain = juce::Decibels::decibelsToGain ((float) attenuation * -0.1f);

                    synth.addSound (sound);
                    addedAny = true;
                }
            }
        }

        return addedAny;
    }

    bool loadSfzInto (const juce::File& file,
                      juce::Synthesiser& synth,
                      juce::AudioFormatManager& formatManager)
    {
        const auto text = file.loadFileAsString();

        if (text.isEmpty())
            return false;

        juce::StringArray lines;
        lines.addLines (text);

        std::map<juce::String, juce::String> region;
        bool addedAny = false;

        auto flushRegion = [&]()
        {
            if (region.empty())
                return;

            const auto samplePath = region["sample"];

            if (samplePath.isEmpty())
            {
                region.clear();
                return;
            }

            const auto sampleFile = file.getParentDirectory().getChildFile (samplePath.unquoted());

            if (! sampleFile.existsAsFile())
            {
                region.clear();
                return;
            }

            juce::AudioBuffer<float> buffer;
            double sampleRate = 44100.0;

            if (! loadAudioFileToBuffer (formatManager, sampleFile, buffer, sampleRate))
            {
                region.clear();
                return;
            }

            auto* sound = new UniversalSound (sampleFile.getFileNameWithoutExtension());
            sound->sampleBuffer = std::move (buffer);
            sound->sourceSampleRate = sampleRate;

            const auto rootText = region.count ("pitch_keycenter") ? region["pitch_keycenter"]
                                  : region.count ("key") ? region["key"]
                                  : juce::String ("60");

            sound->rootNote = guessRootNoteFromName (rootText);

            const auto loText = region.count ("lokey") ? region["lokey"] : rootText;
            const auto hiText = region.count ("hikey") ? region["hikey"] : rootText;

            sound->lowNote = juce::jlimit (0, 127, guessRootNoteFromName (loText));
            sound->highNote = juce::jlimit (0, 127, guessRootNoteFromName (hiText));

            if (sound->lowNote > sound->highNote)
                std::swap (sound->lowNote, sound->highNote);

            const auto loopMode = region.count ("loop_mode") ? region["loop_mode"].toLowerCase()
                                                             : juce::String();

            sound->loopEnabled = loopMode.contains ("loop");

            if (sound->loopEnabled)
            {
                const int numSamples = sound->sampleBuffer.getNumSamples();

                sound->loopStart = region.count ("loop_start") ? region["loop_start"].getIntValue() : 0;
                sound->loopEnd = region.count ("loop_end") ? region["loop_end"].getIntValue() : numSamples;

                sound->loopStart = juce::jlimit (0, numSamples - 1, sound->loopStart);
                sound->loopEnd = juce::jlimit (sound->loopStart + 1, numSamples, sound->loopEnd);
            }

            sound->tuneCents = region.count ("tune") ? region["tune"].getFloatValue() : 0.0f;

            if (region.count ("volume"))
                sound->gain = juce::Decibels::decibelsToGain (region["volume"].getFloatValue());

            synth.addSound (sound);
            addedAny = true;
            region.clear();
        };

        for (auto line : lines)
        {
            line = line.upToFirstOccurrenceOf ("//", false, false).trim();

            if (line.isEmpty())
                continue;

            if (line.startsWith ("<"))
            {
                flushRegion();

                const int close = line.indexOfChar ('>');

                if (close >= 0)
                    line = line.substring (close + 1).trim();

                if (line.isEmpty())
                    continue;
            }

            juce::StringArray tokens;
            tokens.addTokens (line, " \t", "\"");

            for (const auto& token : tokens)
            {
                const int eq = token.indexOfChar ('=');

                if (eq > 0)
                {
                    const auto key = token.substring (0, eq).trim().toLowerCase();
                    const auto value = token.substring (eq + 1).trim().unquoted();
                    region[key] = value;
                }
            }
        }

        flushRegion();
        return addedAny;
    }
}

//==============================================================================
UniversalVoice::UniversalVoice (VoiceParams& paramsIn) : params (paramsIn)
{
}

void UniversalVoice::startNote (int midiNoteNumber,
                                float velocity,
                                juce::SynthesiserSound* sound,
                                int currentPitchWheelPosition)
{
    currentSound = dynamic_cast<UniversalSound*> (sound);

    if (currentSound == nullptr)
        return;

    midiNote = midiNoteNumber;
    velocityGain = 0.3f + 0.7f * velocity;
    samplePos = 0.0;
    modalTime = 0.0;
    modalActive = false;

    pitchWheelMoved (currentPitchWheelPosition);

    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;

    if (lastSampleRate != sr)
    {
        adsr.setSampleRate (sr);
        updateFilters (sr);
        lastSampleRate = sr;
    }

    const float attack  = params.attack  ? params.attack->load()  : 0.01f;
    const float decay   = params.decay   ? params.decay->load()   : 0.2f;
    const float sustain = params.sustain ? params.sustain->load() : 0.8f;
    const float release = params.release ? params.release->load() : 0.3f;

    adsr.setParameters ({ attack, decay, sustain, release });
    adsr.noteOn();

    if (currentSound->isModal || currentSound->sampleBuffer.getNumSamples() == 0)
    {
        initModal (midiNoteNumber);
        modalActive = true;
        return;
    }

    const double semitones = (double) (midiNoteNumber - currentSound->rootNote)
                           + (double) currentSound->tuneCents / 100.0;

    pitchRatio = std::pow (2.0, semitones / 12.0)
               * (currentSound->sourceSampleRate / sr);
}

void UniversalVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void UniversalVoice::pitchWheelMoved (int newValue)
{
    pitchWheelSemitones = ((float) newValue - 8192.0f) / 8192.0f * 2.0f;

    if (currentSound != nullptr && ! currentSound->isModal
        && currentSound->sampleBuffer.getNumSamples() > 0)
    {
        const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        const double semitones = (double) (midiNote - currentSound->rootNote)
                               + (double) currentSound->tuneCents / 100.0
                               + (double) pitchWheelSemitones;

        pitchRatio = std::pow (2.0, semitones / 12.0)
                   * (currentSound->sourceSampleRate / sr);
    }
}

void UniversalVoice::initModal (int noteNumber)
{
    modalTime = 0.0;

    const double baseFreq = juce::MidiMessage::getMidiNoteInHertz (noteNumber);

    const double ratios[numModes]  = { 1.0, 2.01, 3.03, 4.07, 5.11, 6.17 };
    const double amps[numModes]    = { 1.0, 0.55, 0.35, 0.24, 0.17, 0.12 };
    const double decays[numModes]  = { 1.4, 2.1, 3.0, 4.2, 5.6, 7.2 };

    for (int i = 0; i < numModes; ++i)
    {
        modalFreq[i] = baseFreq * ratios[i];
        modalPhase[i] = juce::MathConstants<double>::twoPi
                      * juce::Random::getSystemRandom().nextDouble();
        modalAmp[i] = amps[i] / (double) numModes;
        modalDecay[i] = decays[i];
    }
}

void UniversalVoice::updateFilters (double sr)
{
    const float cutoff = params.cutoff ? params.cutoff->load() : 8000.0f;
    const double safeCutoff = juce::jlimit (20.0, sr * 0.49, (double) cutoff);

    const auto coeffs = juce::IIRCoefficients::makeLowPass (sr, safeCutoff, 0.7071);

    filterL.setCoefficients (coeffs);
    filterR.setCoefficients (coeffs);
}

float UniversalVoice::readSample (int channel, double position) const
{
    if (currentSound == nullptr || currentSound->sampleBuffer.getNumSamples() == 0)
        return 0.0f;

    const int numChannels = currentSound->sampleBuffer.getNumChannels();
    const int numSamples = currentSound->sampleBuffer.getNumSamples();

    const int ch = juce::jlimit (0, numChannels - 1, channel);

    const int i0 = (int) std::floor (position);
    const int i1 = i0 + 1;

    if (i0 < 0 || i0 >= numSamples)
        return 0.0f;

    const float frac = (float) (position - (double) i0);
    const float a = currentSound->sampleBuffer.getSample (ch, i0);
    const float b = currentSound->sampleBuffer.getSample (ch, juce::jlimit (0, numSamples - 1, i1));

    return a + (b - a) * frac;
}

void UniversalVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                      int startSample,
                                      int numSamples)
{
    if (currentSound == nullptr || numSamples <= 0)
        return;

    const double sr = getSampleRate();

    if (sr <= 0.0)
        return;

    if (lastSampleRate != sr)
    {
        adsr.setSampleRate (sr);
        updateFilters (sr);
        lastSampleRate = sr;
    }

    const float attack  = params.attack  ? params.attack->load()  : 0.01f;
    const float decay   = params.decay   ? params.decay->load()   : 0.2f;
    const float sustain = params.sustain ? params.sustain->load() : 0.8f;
    const float release = params.release ? params.release->load() : 0.3f;

    adsr.setParameters ({ attack, decay, sustain, release });

    updateFilters (sr);

    const int outChannels = outputBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (! adsr.isActive())
        {
            clearCurrentNote();
            break;
        }

        const float env = adsr.getNextSample();

        float left = 0.0f;
        float right = 0.0f;

        if (modalActive)
        {
            modalTime += 1.0 / sr;

            double modalSample = 0.0;

            for (int m = 0; m < numModes; ++m)
            {
                const double decay = std::exp (-modalDecay[m] * modalTime);
                modalSample += modalAmp[m] * std::sin (modalPhase[m]) * decay;
                modalPhase[m] += juce::MathConstants<double>::twoPi * modalFreq[m] / sr;
            }

            left = right = (float) modalSample;

            if (modalTime > 6.0 && env < 0.001f)
            {
                clearCurrentNote();
                break;
            }
        }
        else
        {
            const int numSamplesInSound = currentSound->sampleBuffer.getNumSamples();

            if (currentSound->loopEnabled
                && currentSound->loopEnd > currentSound->loopStart
                && samplePos >= (double) currentSound->loopEnd)
            {
                const double loopLen = (double) (currentSound->loopEnd - currentSound->loopStart);
                samplePos = (double) currentSound->loopStart
                          + std::fmod (samplePos - (double) currentSound->loopStart, loopLen);
            }

            if (! currentSound->loopEnabled && samplePos >= (double) numSamplesInSound)
            {
                clearCurrentNote();
                break;
            }

            left = readSample (0, samplePos);
            right = currentSound->sampleBuffer.getNumChannels() > 1
                        ? readSample (1, samplePos)
                        : left;

            samplePos += pitchRatio;
        }

        left = filterL.processSingleSampleRaw (left);
        right = filterR.processSingleSampleRaw (right);

        const float gain = env * velocityGain * currentSound->gain;

        for (int ch = 0; ch < outChannels; ++ch)
        {
            const float value = (ch == 1 ? right : left) * gain;
            outputBuffer.addSample (ch, startSample + i, value);
        }
    }
}

//==============================================================================
DarkTrapAudioProcessor::DarkTrapAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    formatManager.registerBasicFormats();

    voiceParams.cutoff  = apvts.getRawParameterValue ("cutoff");
    voiceParams.attack  = apvts.getRawParameterValue ("attack");
    voiceParams.decay   = apvts.getRawParameterValue ("decay");
    voiceParams.sustain = apvts.getRawParameterValue ("sustain");
    voiceParams.release = apvts.getRawParameterValue ("release");

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new UniversalVoice (voiceParams));

    synth.setNoteStealingEnabled (true);

    createDefaultModalSound();

    reverb.reset();
}

juce::AudioProcessorValueTreeState::ParameterLayout
DarkTrapAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cutoff", 1 }, "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 8000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.4f), 0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.4f), 0.2f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.4f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb", 1 }, "Reverb",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delay", 1 }, "Delay",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "wobble", 1 }, "Tape Wobble",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.08f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "halfTime", 1 }, "Half-Time", false));

    return layout;
}

void DarkTrapAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    juce::ScopedLock sl (synthLock);

    synth.setCurrentPlaybackSampleRate (sampleRate);

    const int maxDelay = juce::jmax (1, (int) (2.0 * sampleRate));
    delayBuffer.setSize (2, maxDelay);
    delayBuffer.clear();
    delayWritePos = 0;

    const int maxWobble = juce::jmax (1, (int) (0.05 * sampleRate));
    wobbleBuffer.setSize (2, maxWobble);
    wobbleBuffer.clear();
    wobbleWritePos = 0;
    wobblePhase = 0.0f;

    reverb.reset();

    generatedPlaybackTime = 0.0;
}

bool DarkTrapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void DarkTrapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ScopedLock sl (synthLock);

    const int numSamples = buffer.getNumSamples();
    const double sr = getSampleRate();

    if (numSamples <= 0 || sr <= 0.0)
        return;

    buffer.clear();

    juce::MidiBuffer combinedMidi;

    for (const auto metadata : midiMessages)
        combinedMidi.addEvent (metadata.getMessage(), metadata.samplePosition);

    if (auditioning && generatedLengthSeconds > 0.0 && ! generatedEvents.empty())
    {
        const double blockStart = generatedPlaybackTime;
        const double blockEnd = blockStart + (double) numSamples / sr;

        for (const auto& event : generatedEvents)
        {
            if (event.timeSeconds >= blockStart && event.timeSeconds < blockEnd)
            {
                int offset = (int) ((event.timeSeconds - blockStart) * sr);
                offset = juce::jlimit (0, numSamples - 1, offset);
                combinedMidi.addEvent (event.message, offset);
            }
        }

        generatedPlaybackTime += (double) numSamples / sr;

        while (generatedPlaybackTime >= generatedLengthSeconds)
            generatedPlaybackTime -= generatedLengthSeconds;
    }

    synth.renderNextBlock (buffer, combinedMidi, 0, numSamples);

    //==========================================================================
    // Reverb
    {
        const float mix = apvts.getRawParameterValue ("reverb")->load();

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.35f + 0.55f * mix;
        rp.damping = 0.45f;
        rp.wetLevel = mix * 0.85f;
        rp.dryLevel = 1.0f - mix * 0.45f;
        rp.width = 0.9f;
        reverb.setParameters (rp);

        if (buffer.getNumChannels() >= 2)
            reverb.processStereo (buffer.getWritePointer (0),
                                  buffer.getWritePointer (1),
                                  numSamples);
        else if (buffer.getNumChannels() == 1)
            reverb.processMono (buffer.getWritePointer (0), numSamples);
    }

    //==========================================================================
    // Delay
    {
        const float mix = apvts.getRawParameterValue ("delay")->load();

        if (mix > 0.001f && delayBuffer.getNumSamples() > 1)
        {
            const int maxDelay = delayBuffer.getNumSamples();
            const int delaySamples = juce::jlimit (1, maxDelay - 1,
                                                    (int) (0.080357f * (float) sr));

            const int numCh = juce::jmin (2, buffer.getNumChannels());

            for (int i = 0; i < numSamples; ++i)
            {
                int readPos = delayWritePos - delaySamples;

                if (readPos < 0)
                    readPos += maxDelay;

                for (int ch = 0; ch < numCh; ++ch)
                {
                    const float dry = buffer.getSample (ch, i);
                    const float delayed = delayBuffer.getSample (ch, readPos);

                    delayBuffer.setSample (ch, delayWritePos, dry + delayed * 0.35f);
                    buffer.setSample (ch, i, dry * (1.0f - mix) + delayed * mix);
                }

                delayWritePos = (delayWritePos + 1) % maxDelay;
            }
        }
    }

    //==========================================================================
    // Tape wobble (modulated short delay)
    {
        const float mix = apvts.getRawParameterValue ("wobble")->load();

        if (mix > 0.001f && wobbleBuffer.getNumSamples() > 8)
        {
            const int maxWobble = wobbleBuffer.getNumSamples();
            const int numCh = juce::jmin (2, buffer.getNumChannels());
            const float depthSamples = juce::jmax (1.0f, (float) sr * 0.003f);

            for (int i = 0; i < numSamples; ++i)
            {
                wobblePhase += juce::MathConstants<float>::twoPi * 0.8f / (float) sr;

                if (wobblePhase > juce::MathConstants<float>::twoPi)
                    wobblePhase -= juce::MathConstants<float>::twoPi;

                const float modulation = std::sin (wobblePhase) * depthSamples;

                float readPos = (float) wobbleWritePos - modulation;

                while (readPos < 0.0f)
                    readPos += (float) maxWobble;

                while (readPos >= (float) maxWobble)
                    readPos -= (float) maxWobble;

                const int r0 = (int) std::floor (readPos);
                const int r1 = (r0 + 1) % maxWobble;
                const float frac = readPos - (float) r0;

                for (int ch = 0; ch < numCh; ++ch)
                {
                    const float dry = buffer.getSample (ch, i);
                    const float wet = wobbleBuffer.getSample (ch, r0) * (1.0f - frac)
                                    + wobbleBuffer.getSample (ch, r1) * frac;

                    wobbleBuffer.setSample (ch, wobbleWritePos, dry);
                    buffer.setSample (ch, i, dry * (1.0f - mix * 0.5f) + wet * mix * 0.5f);
                }

                wobbleWritePos = (wobbleWritePos + 1) % maxWobble;
            }
        }
    }
}

//==============================================================================
void DarkTrapAudioProcessor::resetSounds()
{
    synth.allNotesOff (0, false);
    synth.clearVoices();
    synth.clearSounds();

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new UniversalVoice (voiceParams));
}

void DarkTrapAudioProcessor::createDefaultModalSound()
{
    auto* modal = new UniversalSound ("Modal Physical Model");
    modal->isModal = true;
    modal->lowNote = 0;
    modal->highNote = 127;
    synth.addSound (modal);
}

void DarkTrapAudioProcessor::loadSf2 (const juce::File& file)
{
    loadSf2Into (file, synth);
}

void DarkTrapAudioProcessor::loadSfz (const juce::File& file)
{
    loadSfzInto (file, synth, formatManager);
}

void DarkTrapAudioProcessor::loadWavFiles (const juce::Array<juce::File>& files)
{
    const bool multiSample = files.size() > 1;

    for (const auto& file : files)
    {
        juce::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;

        if (! loadAudioFileToBuffer (formatManager, file, buffer, sampleRate))
            continue;

        auto* sound = new UniversalSound (file.getFileNameWithoutExtension());
        sound->sampleBuffer = std::move (buffer);
        sound->sourceSampleRate = sampleRate;
        sound->rootNote = guessRootNoteFromName (file.getFileNameWithoutExtension());
        sound->loopEnabled = false;

        if (multiSample)
        {
            sound->lowNote = juce::jlimit (0, 127, sound->rootNote - 3);
            sound->highNote = juce::jlimit (0, 127, sound->rootNote + 3);
        }
        else
        {
            sound->lowNote = 0;
            sound->highNote = 127;
        }

        synth.addSound (sound);
    }
}

void DarkTrapAudioProcessor::loadInstrumentBank (const juce::Array<juce::File>& files)
{
    if (files.isEmpty())
        return;

    juce::ScopedLock sl (synthLock);

    resetSounds();

    const auto first = files[0];
    const auto ext = first.getFileExtension().toLowerCase();

    if (ext == ".sf2")
    {
        loadSf2 (first);
    }
    else if (ext == ".sfz")
    {
        loadSfz (first);
    }
    else
    {
        loadWavFiles (files);
    }

    if (synth.getNumSounds() == 0)
        createDefaultModalSound();
}

void DarkTrapAudioProcessor::loadPreset (int index)
{
    auto setFloat = [this] (const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    switch (index)
    {
        case 0: // Init
            setFloat ("cutoff", 8000.0f);
            setFloat ("reverb", 0.20f);
            setFloat ("delay", 0.15f);
            setFloat ("wobble", 0.05f);
            setFloat ("attack", 0.01f);
            setFloat ("decay", 0.20f);
            setFloat ("sustain", 0.80f);
            setFloat ("release", 0.30f);
            break;

        case 1: // Dark Bell
            setFloat ("cutoff", 3500.0f);
            setFloat ("reverb", 0.45f);
            setFloat ("delay", 0.30f);
            setFloat ("wobble", 0.10f);
            setFloat ("attack", 0.005f);
            setFloat ("decay", 0.80f);
            setFloat ("sustain", 0.20f);
            setFloat ("release", 1.50f);
            break;

        case 2: // Ghost Pluck
            setFloat ("cutoff", 6000.0f);
            setFloat ("reverb", 0.35f);
            setFloat ("delay", 0.25f);
            setFloat ("wobble", 0.15f);
            setFloat ("attack", 0.001f);
            setFloat ("decay", 0.15f);
            setFloat ("sustain", 0.0f);
            setFloat ("release", 0.40f);
            break;

        case 3: // Sub Bass
            setFloat ("cutoff", 800.0f);
            setFloat ("reverb", 0.10f);
            setFloat ("delay", 0.05f);
            setFloat ("wobble", 0.02f);
            setFloat ("attack", 0.02f);
            setFloat ("decay", 0.30f);
            setFloat ("sustain", 0.90f);
            setFloat ("release", 0.20f);
            break;

        case 4: // Tape Keys
            setFloat ("cutoff", 4500.0f);
            setFloat ("reverb", 0.50f);
            setFloat ("delay", 0.40f);
            setFloat ("wobble", 0.50f);
            setFloat ("attack", 0.02f);
            setFloat ("decay", 0.60f);
            setFloat ("sustain", 0.50f);
            setFloat ("release", 1.00f);
            break;

        default:
            break;
    }
}

//==============================================================================
void DarkTrapAudioProcessor::generateMelody()
{
    juce::ScopedLock sl (synthLock);

    synth.allNotesOff (0, false);

    generatedSequence.clear();
    generatedEvents.clear();

    const bool useMinor = juce::Random::getSystemRandom().nextFloat() < 0.82f;
    const int rootNote = 48 + juce::Random::getSystemRandom().nextInt (12);

    static const int minorIntervals[] = { 0, 2, 3, 5, 7, 8, 10 };
    static const int majorIntervals[] = { 0, 2, 4, 5, 7, 9, 11 };

    const int* intervals = useMinor ? minorIntervals : majorIntervals;

    const bool halfTime = apvts.getRawParameterValue ("halfTime")->load() > 0.5f;
    const double bpm = halfTime ? 70.0 : 140.0;

    const int ticksPer16th = generatedTicksPerQuarterNote / 4;
    const int barTicks = generatedTicksPerQuarterNote * 4;
    const int numBars = 4;

    struct Rhythm { std::vector<int> steps; };

    static const std::vector<Rhythm> rhythms =
    {
        { { 0, 3, 7, 10, 14 } },
        { { 0, 2, 4, 7, 9, 12, 14 } },
        { { 0, 3, 5, 8, 11, 13, 15 } },
        { { 0, 4, 7, 10, 12, 14 } },
        { { 0, 2, 3, 6, 8, 11, 14 } },
        { { 0, 3, 6, 9, 12, 15 } }
    };

    auto& rng = juce::Random::getSystemRandom();

    int scaleDegree = 0;

    for (int bar = 0; bar < numBars; ++bar)
    {
        const auto& rhythm = rhythms[rng.nextInt ((int) rhythms.size())];

        for (size_t s = 0; s < rhythm.steps.size(); ++s)
        {
            int step = rhythm.steps[s];

            if (halfTime && rng.nextFloat() < 0.35f)
                continue;

            if (rng.nextFloat() < 0.12f)
                continue;

            const int tick = bar * barTicks + step * ticksPer16th;

            // Small random walk through the scale, with octave wrapping.
            const int move = rng.nextInt (5) - 2;
            scaleDegree = juce::jlimit (0, 13, scaleDegree + move);

            const int octave = scaleDegree / 7;
            const int degree = scaleDegree % 7;
            int note = rootNote + intervals[degree] + octave * 12;
            note = juce::jlimit (24, 96, note);

            float velocity = 0.62f + 0.33f * rng.nextFloat();

            if (step == 0)
                velocity = juce::jmax (velocity, 0.88f);
            else if (step == 8)
                velocity = juce::jmax (velocity, 0.78f);

            if (rng.nextFloat() < 0.12f)
                velocity *= 0.55f;

            const auto midiVel = (juce::uint8) juce::jlimit (1, 127,
                                                             (int) std::round (velocity * 127.0f));

            const int lengthTicks = rng.nextFloat() < 0.25f
                                        ? ticksPer16th / 2
                                        : (rng.nextFloat() < 0.30f ? ticksPer16th * 2 : ticksPer16th);

            generatedSequence.addEvent (
                juce::MidiMessage::noteOn (1, note, midiVel), tick);

            generatedSequence.addEvent (
                juce::MidiMessage::noteOff (1, note), tick + lengthTicks);

            // Occasional 32nd-note trap roll.
            if (rng.nextFloat() < 0.08f)
            {
                const int rollNote = juce::jlimit (24, 96, note + intervals[rng.nextInt (7)]);
                const int rollTick = tick + ticksPer16th / 2;
                const auto rollVel = (juce::uint8) juce::jlimit (1, 127, (int) (midiVel * 0.75f));

                generatedSequence.addEvent (
                    juce::MidiMessage::noteOn (1, rollNote, rollVel), rollTick);

                generatedSequence.addEvent (
                    juce::MidiMessage::noteOff (1, rollNote), rollTick + ticksPer16th / 2);
            }
        }
    }

    const int totalTicks = numBars * barTicks;

    generatedSequence.addEvent (
        juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0);

    generatedSequence.addEvent (
        juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);

    generatedSequence.addEvent (
        juce::MidiMessage::endOfTrack(), totalTicks);

    generatedSequence.updateMatchedPairs();

    const double secPerTick = (60.0 / bpm) / (double) generatedTicksPerQuarterNote;

    for (int i = 0; i < generatedSequence.getNumEvents(); ++i)
    {
        const auto* event = generatedSequence.getEventPointer (i);

        if (event != nullptr && event->message.isNoteOnOrOff())
            generatedEvents.push_back ({ event->time * secPerTick, event->message });
    }

    generatedLengthSeconds = generatedSequence.getEndTime() * secPerTick;
    generatedPlaybackTime = 0.0;
    generatedTempoBpm = bpm;
    auditioning = true;
}

juce::File DarkTrapAudioProcessor::getGeneratedMidiFile()
{
    if (generatedSequence.getNumEvents() == 0)
        generateMelody();

    juce::ScopedLock sl (synthLock);

    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("DarkTrapGeneratedMelody.mid");

    file.deleteFile();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (generatedTicksPerQuarterNote);
    midiFile.addTrack (generatedSequence);

    if (auto stream = file.createOutputStream())
    {
        midiFile.writeTo (*stream);
        stream->flush();
    }

    return file;
}

//==============================================================================
void DarkTrapAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DarkTrapAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DarkTrapAudioProcessor::createEditor()
{
    return new DarkTrapAudioProcessorEditor (*this);
}