#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>
#include <cmath>
#include <map>
#include <algorithm>

//==============================================================================
// 16 procedurally-synthesized dark instrument presets.
//==============================================================================
static const DarkInstrumentDef darkInstruments[] =
{
    // name              cat      np  ratios                                                    amps                                     decays                                      noise drive  lo  hi    A      D     S     R     cutoff
    { "Cathedral Bell",  "MEL",   7,  { 1.0, 2.0, 2.4, 3.0, 4.2, 5.4, 6.8, 8.1 },
                                       { 1.0, 0.6, 0.4, 0.5, 0.3, 0.2, 0.15, 0.1 },
                                       { 4.0, 3.5, 3.0, 2.5, 2.0, 1.8, 1.5, 1.3 },
                                       0.02f, 0.05f,  0, 127,  0.005f, 2.0f, 0.55f, 4.0f, 4000.0f },
    { "Ice Bell",        "MEL",   5,  { 1.0, 2.76, 5.40, 8.93, 13.34, 0, 0, 0 },
                                       { 1.0, 0.6, 0.4, 0.25, 0.15, 0, 0, 0 },
                                       { 1.5, 1.2, 0.9, 0.6, 0.4, 0, 0, 0 },
                                       0.0f, 0.0f,  0, 127,  0.001f, 1.0f, 0.30f, 1.5f, 8000.0f },
    { "Ghost Pad",       "CHD",   7,  { 1.0, 1.01, 1.99, 2.01, 3.0, 4.02, 5.0, 0 },
                                       { 0.7, 0.7, 0.4, 0.4, 0.25, 0.15, 0.1, 0 },
                                       { 25.0, 25.0, 22.0, 22.0, 20.0, 18.0, 15.0, 0 },
                                       0.05f, 0.02f,  0, 127,  1.5f, 3.0f, 0.75f, 4.0f, 2500.0f },
    { "Horror Pad",      "CHD",   7,  { 0.5, 0.75, 1.0, 1.33, 1.5, 1.99, 2.0, 0 },
                                       { 0.5, 0.4, 0.8, 0.3, 0.4, 0.35, 0.3, 0 },
                                       { 30.0, 28.0, 25.0, 22.0, 20.0, 18.0, 15.0, 0 },
                                       0.15f, 0.05f,  0, 127,  2.0f, 4.0f, 0.65f, 6.0f, 1500.0f },
    { "Sub 808",         "BAS",   3,  { 1.0, 2.0, 3.0, 0, 0, 0, 0, 0 },
                                       { 1.0, 0.15, 0.05, 0, 0, 0, 0, 0 },
                                       { 20.0, 10.0, 5.0, 0, 0, 0, 0, 0 },
                                       0.0f, 0.15f,  0, 60,  0.005f, 0.5f, 0.90f, 0.8f, 400.0f },
    { "Trap 808",        "BAS",   4,  { 1.0, 2.0, 3.02, 4.0, 0, 0, 0, 0 },
                                       { 1.0, 0.35, 0.15, 0.08, 0, 0, 0, 0 },
                                       { 15.0, 9.0, 5.0, 3.5, 0, 0, 0, 0 },
                                       0.02f, 0.35f, 0, 60,  0.003f, 1.2f, 0.75f, 1.0f, 800.0f },
    { "Eerie Pluck",     "MEL",   5,  { 1.0, 2.76, 5.4, 8.93, 11.7, 0, 0, 0 },
                                       { 1.0, 0.4, 0.2, 0.1, 0.05, 0, 0, 0 },
                                       { 0.5, 0.4, 0.3, 0.2, 0.15, 0, 0, 0 },
                                       0.05f, 0.10f,  0, 127,  0.001f, 0.3f, 0.0f, 0.4f, 6000.0f },
    { "Music Box",       "MEL",   5,  { 1.0, 3.83, 6.98, 9.7, 12.5, 0, 0, 0 },
                                       { 1.0, 0.5, 0.3, 0.15, 0.08, 0, 0, 0 },
                                       { 1.2, 0.9, 0.7, 0.5, 0.3, 0, 0, 0 },
                                       0.0f, 0.0f, 36, 127,  0.001f, 1.5f, 0.25f, 2.0f, 9000.0f },
    { "Tape Piano",      "CHD",   6,  { 1.0, 2.0, 3.0, 4.02, 5.01, 6.0, 0, 0 },
                                       { 1.0, 0.5, 0.3, 0.15, 0.1, 0.06, 0, 0 },
                                       { 3.0, 2.0, 1.5, 1.0, 0.8, 0.6, 0, 0 },
                                       0.03f, 0.0f,  0, 127,  0.002f, 1.5f, 0.35f, 2.5f, 5000.0f },
    { "Metallic Hit",    "PERC",  6,  { 1.0, 2.32, 4.25, 6.63, 8.16, 10.5, 0, 0 },
                                       { 1.0, 0.6, 0.4, 0.25, 0.15, 0.10, 0, 0 },
                                       { 1.8, 1.5, 1.2, 0.9, 0.7, 0.5, 0, 0 },
                                       0.08f, 0.20f,  0, 127,  0.001f, 1.5f, 0.0f, 2.0f, 7000.0f },
    { "Creepy Choir",    "CHD",   7,  { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0 },
                                       { 1.0, 0.3, 0.7, 0.15, 0.25, 0.08, 0.1, 0 },
                                       { 25.0, 22.0, 20.0, 18.0, 15.0, 13.0, 11.0, 0 },
                                       0.12f, 0.0f,  0, 127,  1.0f, 2.0f, 0.70f, 3.0f, 2000.0f },
    { "Dark Strings",    "CHD",   8,  { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 },
                                       { 1.0, 0.7, 0.5, 0.4, 0.3, 0.25, 0.2, 0.15 },
                                       { 25.0, 22.0, 20.0, 18.0, 16.0, 14.0, 12.0, 10.0 },
                                       0.04f, 0.0f,  0, 127,  0.8f, 1.5f, 0.85f, 2.5f, 3000.0f },
    { "Whisper",         "PERC",  4,  { 1.0, 1.98, 3.01, 4.02, 0, 0, 0, 0 },
                                       { 0.6, 0.4, 0.3, 0.2, 0, 0, 0, 0 },
                                       { 3.0, 2.5, 2.0, 1.5, 0, 0, 0, 0 },
                                       0.35f, 0.05f,  0, 127,  0.3f, 1.0f, 0.40f, 2.0f, 2500.0f },
    { "Detuned Saw",     "MEL",   8,  { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 },
                                       { 1.0, 0.5, 0.33, 0.25, 0.2, 0.17, 0.14, 0.12 },
                                       { 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0 },
                                       0.03f, 0.10f,  0, 127,  0.01f, 0.5f, 0.85f, 1.0f, 4000.0f },
    { "Glass",           "MEL",   5,  { 1.0, 2.76, 5.40, 8.93, 13.34, 0, 0, 0 },
                                       { 1.0, 0.7, 0.5, 0.3, 0.15, 0, 0, 0 },
                                       { 2.0, 1.5, 1.0, 0.7, 0.5, 0, 0, 0 },
                                       0.0f, 0.0f, 48, 127,  0.001f, 1.5f, 0.30f, 2.0f, 12000.0f },
    { "Bell Bass",       "BAS",   5,  { 0.5, 1.0, 1.99, 3.0, 4.1, 0, 0, 0 },
                                       { 0.4, 1.0, 0.5, 0.3, 0.15, 0, 0, 0 },
                                       { 25.0, 20.0, 18.0, 15.0, 12.0, 0, 0, 0 },
                                       0.02f, 0.10f, 12, 60,  0.005f, 1.5f, 0.60f, 2.5f, 1500.0f }
};

int DarkTrapAudioProcessor::getNumDarkInstruments()
{
    return (int)(sizeof (darkInstruments) / sizeof (darkInstruments[0]));
}

const DarkInstrumentDef& DarkTrapAudioProcessor::getDarkInstrument (int index)
{
    return darkInstruments[juce::jlimit (0, getNumDarkInstruments() - 1, index)];
}

//==============================================================================
// SF2 / SFZ / WAV parsing helpers
namespace
{
    constexpr int GEN_KEY_RANGE = 43, GEN_INSTRUMENT = 41, GEN_SAMPLE_ID = 53;
    constexpr int GEN_OVERRIDING_ROOT_KEY = 58, GEN_SAMPLE_MODES = 54;
    constexpr int GEN_COARSE_TUNE = 51, GEN_FINE_TUNE = 52, GEN_INITIAL_ATTENUATION = 48;

    inline juce::uint16 readU16 (const juce::uint8* p) { return (juce::uint16)(p[0] | (p[1] << 8)); }
    inline juce::uint32 readU32 (const juce::uint8* p)
    {
        return (juce::uint32) p[0] | ((juce::uint32) p[1] << 8)
             | ((juce::uint32) p[2] << 16) | ((juce::uint32) p[3] << 24);
    }
    inline juce::int16 readI16 (const juce::uint8* p) { return (juce::int16) readU16 (p); }

    struct RiffChunk { const juce::uint8* data = nullptr; juce::uint32 size = 0; };

    bool parseSf2Chunks (const juce::uint8* data, size_t size,
                         std::map<juce::String, RiffChunk>& chunks)
    {
        if (size < 12 || std::memcmp (data, "RIFF", 4) != 0
            || std::memcmp (data + 8, "sfbk", 4) != 0) return false;
        size_t pos = 12;
        while (pos + 8 <= size)
        {
            const juce::uint32 sz = readU32 (data + pos + 4);
            if (pos + 8 + sz > size) break;
            const juce::String id ((const char*)(data + pos), 4);
            if (id == "LIST")
            {
                const juce::String lt ((const char*)(data + pos + 8), 4);
                if (lt == "sdta" || lt == "pdta")
                {
                    size_t sp = pos + 12, end = pos + 8 + sz;
                    while (sp + 8 <= end)
                    {
                        const juce::uint32 ss = readU32 (data + sp + 4);
                        if (sp + 8 + ss > end) break;
                        chunks[juce::String ((const char*)(data + sp), 4)] = { data + sp + 8, ss };
                        sp += 8 + ss + (ss & 1);
                    }
                }
            }
            pos += 8 + sz + (sz & 1);
        }
        return true;
    }

    int guessRootNoteFromName (const juce::String& name)
    {
        const auto upper = name.toUpperCase();
        for (int i = 0; i < upper.length(); ++i)
        {
            const auto c = upper[i];
            int semi = -1;
            if (c == 'C') semi = 0; else if (c == 'D') semi = 2;
            else if (c == 'E') semi = 4; else if (c == 'F') semi = 5;
            else if (c == 'G') semi = 7; else if (c == 'A') semi = 9;
            else if (c == 'B') semi = 11;
            if (semi >= 0)
            {
                int idx = i + 1;
                if (idx < upper.length() && (upper[idx] == '#' || upper[idx] == 'S')) { ++semi; ++idx; }
                else if (idx < upper.length() && upper[idx] == 'B') { --semi; ++idx; }
                if (idx < upper.length() && juce::CharacterFunctions::isDigit (upper[idx]))
                    return juce::jlimit (0, 127, 12 * (upper[idx] - '0' + 1) + semi);
            }
        }
        const auto digits = upper.retainCharacters ("0123456789");
        if (digits.isNotEmpty())
        {
            const int v = digits.getIntValue();
            if (v >= 0 && v <= 127) return v;
        }
        return 60;
    }

    bool loadAudioFileToBuffer (juce::AudioFormatManager& fmt, const juce::File& file,
                                juce::AudioBuffer<float>& buffer, double& sr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (fmt.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples <= 0) return false;
        const int nCh = juce::jlimit (1, 2, (int) reader->numChannels);
        const int nS = (int) juce::jmin (reader->lengthInSamples,
                                         (juce::int64) std::numeric_limits<int>::max());
        buffer.setSize (nCh, nS);
        reader->read (&buffer, 0, nS, 0, true, true);
        sr = reader->sampleRate;
        return true;
    }

    bool loadSf2Into (const juce::File& file, juce::Synthesiser& synth)
    {
        juce::MemoryBlock mem;
        file.loadFileAsData (mem);
        const auto* data = static_cast<const juce::uint8*> (mem.getData());
        std::map<juce::String, RiffChunk> chunks;
        if (! parseSf2Chunks (data, mem.getSize(), chunks)) return false;

        auto needs = { "smpl", "phdr", "pbag", "pgen", "inst", "ibag", "igen", "shdr" };
        for (auto* n : needs) if (chunks.find (n) == chunks.end()) return false;

        const auto* smpl = chunks["smpl"].data;
        const auto* phdr = chunks["phdr"].data;
        const auto* pbag = chunks["pbag"].data;
        const auto* pgen = chunks["pgen"].data;
        const auto* inst = chunks["inst"].data;
        const auto* ibag = chunks["ibag"].data;
        const auto* igen = chunks["igen"].data;
        const auto* shdr = chunks["shdr"].data;

        const int smplFrames = (int)(chunks["smpl"].size / 2);
        const int phdrCount = (int)(chunks["phdr"].size / 38);
        const int pbagCount = (int)(chunks["pbag"].size / 4);
        const int pgenCount = (int)(chunks["pgen"].size / 4);
        const int instCount = (int)(chunks["inst"].size / 22);
        const int ibagCount = (int)(chunks["ibag"].size / 4);
        const int igenCount = (int)(chunks["igen"].size / 4);
        const int shdrCount = (int)(chunks["shdr"].size / 46);

        if (phdrCount < 2 || pbagCount < 1 || pgenCount < 1 || instCount < 2
            || ibagCount < 1 || igenCount < 1 || shdrCount < 2 || smplFrames <= 0)
            return false;

        bool addedAny = false;
        for (int pi = 0; pi + 1 < phdrCount; ++pi)
        {
            const int pbagStart = readU16 (phdr + pi * 38 + 24);
            const int pbagEnd = readU16 (phdr + (pi + 1) * 38 + 24);
            for (int pbi = pbagStart; pbi < pbagEnd && pbi < pbagCount - 1; ++pbi)
            {
                const int gStart = readU16 (pbag + pbi * 4);
                const int gEnd = readU16 (pbag + (pbi + 1) * 4);
                int instIdx = -1;
                for (int gi = gStart; gi < gEnd && gi < pgenCount - 1; ++gi)
                    if (readU16 (pgen + gi * 4) == GEN_INSTRUMENT)
                    { instIdx = readU16 (pgen + gi * 4 + 2); break; }
                if (instIdx < 0 || instIdx + 1 >= instCount) continue;

                const int ibagStart = readU16 (inst + instIdx * 22 + 20);
                const int ibagEnd = readU16 (inst + (instIdx + 1) * 22 + 20);
                for (int ibi = ibagStart; ibi < ibagEnd && ibi < ibagCount - 1; ++ibi)
                {
                    const int igStart = readU16 (ibag + ibi * 4);
                    const int igEnd = readU16 (ibag + (ibi + 1) * 4);
                    int sampIdx = -1, rootKey = -1, kLo = 0, kHi = 127;
                    int sampModes = 0, cTune = 0, fTune = 0, atten = 0;
                    for (int gi = igStart; gi < igEnd && gi < igenCount - 1; ++gi)
                    {
                        const int op = readU16 (igen + gi * 4);
                        const int am = readU16 (igen + gi * 4 + 2);
                        switch (op)
                        {
                            case GEN_SAMPLE_ID: sampIdx = am; break;
                            case GEN_KEY_RANGE: kLo = am & 0xFF; kHi = (am >> 8) & 0xFF; break;
                            case GEN_OVERRIDING_ROOT_KEY: rootKey = am; break;
                            case GEN_SAMPLE_MODES: sampModes = am; break;
                            case GEN_COARSE_TUNE: cTune = (juce::int16) am; break;
                            case GEN_FINE_TUNE: fTune = (juce::int16) am; break;
                            case GEN_INITIAL_ATTENUATION: atten = am; break;
                            default: break;
                        }
                    }
                    if (sampIdx < 0 || sampIdx + 1 >= shdrCount) continue;
                    const auto* sh = shdr + sampIdx * 46;
                    const auto sStart = readU32 (sh + 20), sEnd = readU32 (sh + 24);
                    const auto lStart = readU32 (sh + 28), lEnd = readU32 (sh + 32);
                    const auto sRate = readU32 (sh + 36);
                    const auto origPitch = (int) sh[40];
                    const auto pCorr = (int)(juce::int8) sh[41];
                    if (sEnd <= sStart || sEnd > (juce::uint32) smplFrames) continue;
                    const int nFrames = (int)(sEnd - sStart);
                    if (nFrames <= 0) continue;
                    auto* sound = new UniversalSound ("SF2");
                    sound->sampleBuffer.setSize (1, nFrames);
                    for (int i = 0; i < nFrames; ++i)
                        sound->sampleBuffer.setSample (0, i,
                            (float) readI16 (smpl + (sStart + (juce::uint32) i) * 2) / 32768.0f);
                    sound->sourceSampleRate = sRate > 0 ? (double) sRate : 44100.0;
                    sound->rootNote = (rootKey >= 0 && rootKey <= 127) ? rootKey
                                                                     : juce::jlimit (0, 127, origPitch);
                    sound->lowNote = juce::jlimit (0, 127, kLo);
                    sound->highNote = juce::jlimit (0, 127, kHi);
                    if (sound->lowNote > sound->highNote) std::swap (sound->lowNote, sound->highNote);
                    sound->loopEnabled = (sampModes & 1) != 0;
                    if (sound->loopEnabled)
                    {
                        const int ls = (int)(lStart - sStart), le = (int)(lEnd - sStart);
                        if (ls >= 0 && le > ls && le <= nFrames) { sound->loopStart = ls; sound->loopEnd = le; }
                        else { sound->loopStart = 0; sound->loopEnd = nFrames; }
                    }
                    sound->tuneCents = (float)(pCorr + fTune + cTune * 100);
                    sound->gain = juce::Decibels::decibelsToGain ((float) atten * -0.1f);
                    synth.addSound (sound);
                    addedAny = true;
                }
            }
        }
        return addedAny;
    }

    bool loadSfzInto (const juce::File& file, juce::Synthesiser& synth,
                      juce::AudioFormatManager& fmt)
    {
        const auto text = file.loadFileAsString();
        if (text.isEmpty()) return false;
        juce::StringArray lines; lines.addLines (text);
        std::map<juce::String, juce::String> region;
        bool addedAny = false;

        auto flush = [&]() {
            if (region.empty()) return;
            const auto samplePath = region["sample"];
            if (samplePath.isEmpty()) { region.clear(); return; }
            const auto sampleFile = file.getParentDirectory().getChildFile (samplePath.unquoted());
            if (! sampleFile.existsAsFile()) { region.clear(); return; }
            juce::AudioBuffer<float> buf; double sr = 44100.0;
            if (! loadAudioFileToBuffer (fmt, sampleFile, buf, sr)) { region.clear(); return; }
            auto* sound = new UniversalSound (sampleFile.getFileNameWithoutExtension());
            sound->sampleBuffer = std::move (buf);
            sound->sourceSampleRate = sr;
            const auto rootText = region.count ("pitch_keycenter") ? region["pitch_keycenter"]
                                 : region.count ("key") ? region["key"] : juce::String ("60");
            sound->rootNote = guessRootNoteFromName (rootText);
            sound->lowNote = juce::jlimit (0, 127, guessRootNoteFromName (
                region.count ("lokey") ? region["lokey"] : rootText));
            sound->highNote = juce::jlimit (0, 127, guessRootNoteFromName (
                region.count ("hikey") ? region["hikey"] : rootText));
            if (sound->lowNote > sound->highNote) std::swap (sound->lowNote, sound->highNote);
            sound->loopEnabled = region.count ("loop_mode") && region["loop_mode"].contains ("loop");
            if (sound->loopEnabled)
            {
                const int n = sound->sampleBuffer.getNumSamples();
                sound->loopStart = region.count ("loop_start") ? region["loop_start"].getIntValue() : 0;
                sound->loopEnd = region.count ("loop_end") ? region["loop_end"].getIntValue() : n;
            }
            if (region.count ("volume"))
                sound->gain = juce::Decibels::decibelsToGain (region["volume"].getFloatValue());
            synth.addSound (sound);
            addedAny = true;
            region.clear();
        };

        for (auto line : lines)
        {
            line = line.upToFirstOccurrenceOf ("//", false, false).trim();
            if (line.isEmpty()) continue;
            if (line.startsWith ("<"))
            {
                flush();
                const int close = line.indexOfChar ('>');
                if (close >= 0) line = line.substring (close + 1).trim();
                if (line.isEmpty()) continue;
            }
            juce::StringArray tokens; tokens.addTokens (line, " \t", "\"");
            for (const auto& t : tokens)
            {
                const int eq = t.indexOfChar ('=');
                if (eq > 0) region[t.substring (0, eq).trim().toLowerCase()] = t.substring (eq + 1).trim().unquoted();
            }
        }
        flush();
        return addedAny;
    }
}

//==============================================================================
UniversalVoice::UniversalVoice (VoiceParams& paramsIn) : params (paramsIn) {}

void UniversalVoice::startNote (int note, float vel, juce::SynthesiserSound* snd, int pitchWheel)
{
    currentSound = dynamic_cast<UniversalSound*> (snd);
    if (currentSound == nullptr) return;
    midiNote = note;
    velocityGain = 0.3f + 0.7f * vel;
    samplePos = 0.0; modalTime = 0.0; modalActive = false;
    pitchWheelMoved (pitchWheel);

    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    if (lastSampleRate != sr) { adsr.setSampleRate (sr); updateFilters (sr); lastSampleRate = sr; }

    const bool custom = currentSound->useCustomEnvelope;
    const float a = custom ? currentSound->presetAttack  : (params.attack  ? params.attack->load()  : 0.01f);
    const float d = custom ? currentSound->presetDecay   : (params.decay   ? params.decay->load()   : 0.2f);
    const float s = custom ? currentSound->presetSustain : (params.sustain ? params.sustain->load() : 0.8f);
    const float r = custom ? currentSound->presetRelease : (params.release ? params.release->load() : 0.3f);
    adsr.setParameters ({ a, d, s, r });
    adsr.noteOn();

    if (currentSound->isModal || currentSound->sampleBuffer.getNumSamples() == 0)
    {
        initModal (note);
        modalActive = true;
        return;
    }
    const double semis = (double)(note - currentSound->rootNote)
                       + (double) currentSound->tuneCents / 100.0;
    pitchRatio = std::pow (2.0, semis / 12.0) * (currentSound->sourceSampleRate / sr);
}

void UniversalVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff) adsr.noteOff();
    else { adsr.reset(); clearCurrentNote(); }
}

void UniversalVoice::pitchWheelMoved (int v)
{
    pitchWheelSemitones = ((float) v - 8192.0f) / 8192.0f * 2.0f;
    if (currentSound && ! currentSound->isModal && currentSound->sampleBuffer.getNumSamples() > 0)
    {
        const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        const double semis = (double)(midiNote - currentSound->rootNote)
                           + (double) currentSound->tuneCents / 100.0
                           + (double) pitchWheelSemitones;
        pitchRatio = std::pow (2.0, semis / 12.0) * (currentSound->sourceSampleRate / sr);
    }
}

void UniversalVoice::initModal (int noteNumber)
{
    modalTime = 0.0;
    const double baseFreq = juce::MidiMessage::getMidiNoteInHertz (noteNumber);

    numActiveModes = currentSound->numPartials > 0
                     ? juce::jlimit (1, maxModes, currentSound->numPartials)
                     : 6;

    double ampSum = 0.0;
    for (int i = 0; i < numActiveModes; ++i)
    {
        const double r = currentSound->numPartials > 0 ? currentSound->partialRatios[i] : (1.0 + i * 1.05);
        const double a = currentSound->numPartials > 0 ? currentSound->partialAmps[i]   : (1.0 / (i + 1.0));
        const double d = currentSound->numPartials > 0 ? currentSound->partialDecays[i] : (1.4 + i * 1.1);
        modalFreq[i] = baseFreq * r;
        modalPhase[i] = juce::MathConstants<double>::twoPi * juce::Random::getSystemRandom().nextDouble();
        modalAmp[i] = a;
        modalDecay[i] = d;
        ampSum += a;
    }
    if (ampSum > 0.0)
        for (int i = 0; i < numActiveModes; ++i) modalAmp[i] /= ampSum;
}

void UniversalVoice::updateFilters (double sr)
{
    const float c = (currentSound && currentSound->useCustomEnvelope)
                    ? currentSound->presetCutoff
                    : (params.cutoff ? params.cutoff->load() : 8000.0f);
    const double safe = juce::jlimit (20.0, sr * 0.49, (double) c);
    const auto coeffs = juce::IIRCoefficients::makeLowPass (sr, safe, 0.7071);
    filterL.setCoefficients (coeffs);
    filterR.setCoefficients (coeffs);
}

float UniversalVoice::readSample (int ch, double pos) const
{
    if (! currentSound || currentSound->sampleBuffer.getNumSamples() == 0) return 0.0f;
    const int nCh = currentSound->sampleBuffer.getNumChannels();
    const int n = currentSound->sampleBuffer.getNumSamples();
    const int c = juce::jlimit (0, nCh - 1, ch);
    const int i0 = (int) std::floor (pos), i1 = i0 + 1;
    if (i0 < 0 || i0 >= n) return 0.0f;
    const float f = (float)(pos - i0);
    const float a = currentSound->sampleBuffer.getSample (c, i0);
    const float b = currentSound->sampleBuffer.getSample (c, juce::jlimit (0, n - 1, i1));
    return a + (b - a) * f;
}

void UniversalVoice::renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples)
{
    if (! currentSound || numSamples <= 0) return;
    const double sr = getSampleRate();
    if (sr <= 0.0) return;
    if (lastSampleRate != sr) { adsr.setSampleRate (sr); updateFilters (sr); lastSampleRate = sr; }

    const bool custom = currentSound->useCustomEnvelope;
    const float a = custom ? currentSound->presetAttack  : (params.attack  ? params.attack->load()  : 0.01f);
    const float d = custom ? currentSound->presetDecay   : (params.decay   ? params.decay->load()   : 0.2f);
    const float s = custom ? currentSound->presetSustain : (params.sustain ? params.sustain->load() : 0.8f);
    const float r = custom ? currentSound->presetRelease : (params.release ? params.release->load() : 0.3f);
    adsr.setParameters ({ a, d, s, r });
    updateFilters (sr);

    const int nCh = out.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (! adsr.isActive()) { clearCurrentNote(); break; }
        const float env = adsr.getNextSample();
        float L = 0.0f, R = 0.0f;

        if (modalActive)
        {
            modalTime += 1.0 / sr;
            double mix = 0.0;
            for (int m = 0; m < numActiveModes; ++m)
            {
                const double md = std::exp (-modalDecay[m] * modalTime);
                mix += modalAmp[m] * std::sin (modalPhase[m]) * md;
                modalPhase[m] += juce::MathConstants<double>::twoPi * modalFreq[m] / sr;
            }
            if (currentSound->noiseAmount > 0.0f)
            {
                const double noise = juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0;
                mix += noise * currentSound->noiseAmount * 0.5;
            }
            if (currentSound->driveAmount > 0.0f)
            {
                const double drive = 1.0 + currentSound->driveAmount * 8.0;
                mix = std::tanh (mix * drive) / drive;
            }
            L = R = (float) mix;
        }
        else
        {
            const int n = currentSound->sampleBuffer.getNumSamples();
            if (currentSound->loopEnabled && currentSound->loopEnd > currentSound->loopStart
                && samplePos >= (double) currentSound->loopEnd)
            {
                const double len = (double)(currentSound->loopEnd - currentSound->loopStart);
                samplePos = (double) currentSound->loopStart
                          + std::fmod (samplePos - (double) currentSound->loopStart, len);
            }
            if (! currentSound->loopEnabled && samplePos >= (double) n)
            { clearCurrentNote(); break; }

            L = readSample (0, samplePos);
            R = currentSound->sampleBuffer.getNumChannels() > 1 ? readSample (1, samplePos) : L;
            samplePos += pitchRatio;
        }

        L = filterL.processSingleSampleRaw (L);
        R = filterR.processSingleSampleRaw (R);
        const float g = env * velocityGain * currentSound->gain;

        for (int ch = 0; ch < nCh; ++ch)
            out.addSample (ch, startSample + i, (ch == 1 ? R : L) * g);
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

    for (int i = 0; i < 3; ++i) setupLayer (i);

    // Default presets: Cathedral Bell, Ghost Pad, Sub 808
    loadDarkInstrumentPreset (0, 0);
    loadDarkInstrumentPreset (1, 2);
    loadDarkInstrumentPreset (2, 4);
}

juce::AudioProcessorValueTreeState::ParameterLayout
DarkTrapAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "cutoff", 1 },
        "Filter Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 8000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", 1 },
        "Attack", juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.4f), 0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "decay", 1 },
        "Decay", juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.4f), 0.2f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sustain", 1 },
        "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 },
        "Release", juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.4f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "reverb", 1 },
        "Reverb", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "delay", 1 },
        "Delay", juce::NormalisableRange<float> (0.0f, 1.0f), 0.20f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "chorus", 1 },
        "Chorus", juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "distortion", 1 },
        "Distortion", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "flanger", 1 },
        "Flanger", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "halfSpeed", 1 },
        "Half-Speed", false));
    return layout;
}

void DarkTrapAudioProcessor::setupLayer (int index)
{
    auto& layer = layers[(size_t) index];
    layer.synth.setCurrentPlaybackSampleRate (44100.0);
    layer.synth.clearVoices();
    layer.synth.clearSounds();
    for (int v = 0; v < 12; ++v)
        layer.synth.addVoice (new UniversalVoice (voiceParams));
    layer.synth.setNoteStealingEnabled (true);
}

void DarkTrapAudioProcessor::loadDarkInstrumentPreset (int layerIndex, int presetIndex)
{
    if (layerIndex < 0 || layerIndex > 2) return;
    presetIndex = juce::jlimit (0, getNumDarkInstruments() - 1, presetIndex);

    juce::ScopedLock sl (synthLock);
    auto& layer = layers[(size_t) layerIndex];
    const auto& def = darkInstruments[presetIndex];

    layer.synth.allNotesOff (0, false);
    layer.synth.clearSounds();

    auto* sound = new UniversalSound (def.name);
    sound->isModal = true;
    sound->numPartials = def.numPartials;
    for (int i = 0; i < 8; ++i)
    {
        sound->partialRatios[i] = def.ratios[i];
        sound->partialAmps[i]   = def.amps[i];
        sound->partialDecays[i] = def.decays[i];
    }
    sound->noiseAmount = def.noiseAmount;
    sound->driveAmount = def.driveAmount;
    sound->lowNote = def.lowNote;
    sound->highNote = def.highNote;
    sound->gain = 1.0f;
    sound->useCustomEnvelope = true;
    sound->presetAttack = def.attack;
    sound->presetDecay = def.decay;
    sound->presetSustain = def.sustain;
    sound->presetRelease = def.release;
    sound->presetCutoff = def.cutoff;

    layer.synth.addSound (sound);
    layer.sourceName = def.name;
    layer.presetIndex = presetIndex;
}

void DarkTrapAudioProcessor::loadSoundIntoLayer (int layerIndex, const juce::Array<juce::File>& files)
{
    if (files.isEmpty() || layerIndex < 0 || layerIndex > 2) return;
    juce::ScopedLock sl (synthLock);
    auto& layer = layers[(size_t) layerIndex];

    layer.synth.allNotesOff (0, false);
    layer.synth.clearSounds();

    const auto first = files[0];
    const auto ext = first.getFileExtension().toLowerCase();
    bool loaded = false;

    if (ext == ".sf2") loaded = loadSf2Into (first, layer.synth);
    else if (ext == ".sfz") loaded = loadSfzInto (first, layer.synth, formatManager);
    else
    {
        const bool multi = files.size() > 1;
        for (const auto& file : files)
        {
            juce::AudioBuffer<float> buf; double sr = 44100.0;
            if (! loadAudioFileToBuffer (formatManager, file, buf, sr)) continue;
            auto* s = new UniversalSound (file.getFileNameWithoutExtension());
            s->sampleBuffer = std::move (buf);
            s->sourceSampleRate = sr;
            s->rootNote = guessRootNoteFromName (file.getFileNameWithoutExtension());
            if (multi)
            {
                s->lowNote = juce::jlimit (0, 127, s->rootNote - 3);
                s->highNote = juce::jlimit (0, 127, s->rootNote + 3);
            }
            layer.synth.addSound (s);
            loaded = true;
        }
    }

    if (loaded) layer.sourceName = first.getFileNameWithoutExtension();
    else
    {
        // Fall back to Cathedral Bell
        loadDarkInstrumentPreset (layerIndex, 0);
    }
}

void DarkTrapAudioProcessor::prepareToPlay (double sr, int)
{
    juce::ScopedLock sl (synthLock);
    for (auto& l : layers) l.synth.setCurrentPlaybackSampleRate (sr);

    delayBuffer.setSize (2, juce::jmax (1, (int)(2.0 * sr))); delayBuffer.clear(); delayWritePos = 0;
    chorusBuffer.setSize (2, juce::jmax (1, (int)(0.05 * sr))); chorusBuffer.clear();
    chorusWritePos = 0; chorusPhase = 0.0f;
    flangerBuffer.setSize (2, juce::jmax (1, (int)(0.02 * sr))); flangerBuffer.clear();
    flangerWritePos = 0; flangerPhase = 0.0f;
    halfSpeedBuffer.setSize (2, juce::jmax (1, (int)(3.0 * sr))); halfSpeedBuffer.clear();
    halfSpeedWritePos = 0; halfSpeedReadPos = 0.0;
    reverb.reset();
    generatedPlaybackTime = 0.0;
}

bool DarkTrapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DarkTrapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ScopedLock sl (synthLock);

    const int nS = buffer.getNumSamples();
    const double sr = getSampleRate();
    if (nS <= 0 || sr <= 0.0) return;
    buffer.clear();

    const auto* variation = (! variations.empty() && currentVariationIndex < (int) variations.size())
                            ? &variations[(size_t) currentVariationIndex] : nullptr;

    juce::AudioBuffer<float> layerBuffer (2, nS);
    const float vols[3] = { layer0Vol.load(), layer1Vol.load(), layer2Vol.load() };

    for (int li = 0; li < 3; ++li)
    {
        auto& layer = layers[(size_t) li];
        if (layer.muted || layer.volume < 0.001f) continue;
        layerBuffer.clear();

        juce::MidiBuffer layerMidi;
        for (const auto md : midi) layerMidi.addEvent (md.getMessage(), md.samplePosition);

        if (auditioning && variation)
        {
            const auto& seq = (li == 0) ? variation->melodySeq
                            : (li == 1) ? variation->chordSeq
                            : variation->bassSeq;
            const double secPerTick = (60.0 / variation->bpm) / (double) generatedTicksPerQuarterNote;
            const double blockStart = generatedPlaybackTime;
            const double blockEnd = blockStart + (double) nS / sr;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                const auto* ev = seq.getEventPointer (i);
                if (! ev || ! ev->message.isNoteOnOrOff()) continue;
                const double t = seq.getEventTime (i) * secPerTick;
                if (t >= blockStart && t < blockEnd)
                    layerMidi.addEvent (ev->message,
                        juce::jlimit (0, nS - 1, (int)((t - blockStart) * sr)));
            }
        }

        layer.synth.renderNextBlock (layerBuffer, layerMidi, 0, nS);
        const float g = layer.volume * vols[li];
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* src = layerBuffer.getReadPointer (juce::jmin (ch, 1));
            float* dst = buffer.getWritePointer (ch);
            for (int i = 0; i < nS; ++i) dst[i] += src[i] * g;
        }
    }

    if (auditioning && variation)
    {
        generatedPlaybackTime += (double) nS / sr;
        const double total = (60.0 / variation->bpm) * variation->bars * 4.0;
        if (total > 0.0) while (generatedPlaybackTime >= total) generatedPlaybackTime -= total;
    }

    const int nCh = juce::jmin (2, buffer.getNumChannels());

    const float dist = apvts.getRawParameterValue ("distortion")->load();
    if (dist > 0.001f)
    {
        const float drive = 1.0f + dist * 12.0f;
        const float norm = 1.0f / std::tanh (drive);
        for (int ch = 0; ch < nCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            for (int i = 0; i < nS; ++i) d[i] = std::tanh (d[i] * drive) * norm;
        }
    }

    const float chorusMix = apvts.getRawParameterValue ("chorus")->load();
    if (chorusMix > 0.001f && chorusBuffer.getNumSamples() > 100)
    {
        const int mx = chorusBuffer.getNumSamples();
        for (int i = 0; i < nS; ++i)
        {
            chorusPhase += juce::MathConstants<float>::twoPi * 0.5f / (float) sr;
            if (chorusPhase > juce::MathConstants<float>::twoPi) chorusPhase -= juce::MathConstants<float>::twoPi;
            const float ms = 15.0f + 5.0f * std::sin (chorusPhase);
            int ds = juce::jlimit (1, mx - 1, (int)(ms * 0.001f * (float) sr));
            int rp = chorusWritePos - ds; while (rp < 0) rp += mx;
            for (int ch = 0; ch < nCh; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                const float wet = chorusBuffer.getSample (ch, rp);
                chorusBuffer.setSample (ch, chorusWritePos, dry);
                buffer.setSample (ch, i, dry * (1.0f - chorusMix * 0.5f) + wet * chorusMix * 0.5f);
            }
            chorusWritePos = (chorusWritePos + 1) % mx;
        }
    }

    const float flangerMix = apvts.getRawParameterValue ("flanger")->load();
    if (flangerMix > 0.001f && flangerBuffer.getNumSamples() > 100)
    {
        const int mx = flangerBuffer.getNumSamples();
        for (int i = 0; i < nS; ++i)
        {
            flangerPhase += juce::MathConstants<float>::twoPi * 0.2f / (float) sr;
            if (flangerPhase > juce::MathConstants<float>::twoPi) flangerPhase -= juce::MathConstants<float>::twoPi;
            const float ms = 3.0f + 2.0f * std::sin (flangerPhase);
            int ds = juce::jlimit (1, mx - 1, (int)(ms * 0.001f * (float) sr));
            int rp = flangerWritePos - ds; while (rp < 0) rp += mx;
            for (int ch = 0; ch < nCh; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                const float wet = flangerBuffer.getSample (ch, rp);
                flangerBuffer.setSample (ch, flangerWritePos, dry + wet * 0.5f);
                buffer.setSample (ch, i, dry * (1.0f - flangerMix * 0.5f) + wet * flangerMix * 0.5f);
            }
            flangerWritePos = (flangerWritePos + 1) % mx;
        }
    }

    const float delayMix = apvts.getRawParameterValue ("delay")->load();
    if (delayMix > 0.001f && delayBuffer.getNumSamples() > 1)
    {
        const int mx = delayBuffer.getNumSamples();
        const int ds = juce::jlimit (1, mx - 1, (int)(0.080357f * (float) sr));
        for (int i = 0; i < nS; ++i)
        {
            int rp = delayWritePos - ds; if (rp < 0) rp += mx;
            for (int ch = 0; ch < nCh; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                const float w = delayBuffer.getSample (ch, rp);
                delayBuffer.setSample (ch, delayWritePos, dry + w * 0.35f);
                buffer.setSample (ch, i, dry * (1.0f - delayMix) + w * delayMix);
            }
            delayWritePos = (delayWritePos + 1) % mx;
        }
    }

    {
        const float mix = apvts.getRawParameterValue ("reverb")->load();
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.5f + 0.5f * mix;
        rp.damping = 0.45f;
        rp.wetLevel = mix * 0.85f;
        rp.dryLevel = 1.0f - mix * 0.45f;
        rp.width = 0.95f;
        reverb.setParameters (rp);
        if (buffer.getNumChannels() >= 2)
            reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), nS);
        else if (buffer.getNumChannels() == 1)
            reverb.processMono (buffer.getWritePointer (0), nS);
    }

    if (apvts.getRawParameterValue ("halfSpeed")->load() > 0.5f && halfSpeedBuffer.getNumSamples() > 1000)
    {
        const int mx = halfSpeedBuffer.getNumSamples();
        for (int i = 0; i < nS; ++i)
            for (int ch = 0; ch < nCh; ++ch)
                halfSpeedBuffer.setSample (ch, (halfSpeedWritePos + i) % mx, buffer.getSample (ch, i));
        halfSpeedWritePos = (halfSpeedWritePos + nS) % mx;
        for (int i = 0; i < nS; ++i)
        {
            const int i0 = (int) halfSpeedReadPos;
            const int i1 = (i0 + 1) % mx;
            const float f = (float)(halfSpeedReadPos - (double) i0);
            for (int ch = 0; ch < nCh; ++ch)
            {
                const float a = halfSpeedBuffer.getSample (ch, i0);
                const float b = halfSpeedBuffer.getSample (ch, i1);
                buffer.setSample (ch, i, a * (1.0f - f) + b * f);
            }
            halfSpeedReadPos += 0.5;
            if (halfSpeedReadPos >= mx) halfSpeedReadPos -= mx;
        }
    }
}

//==============================================================================
void DarkTrapAudioProcessor::setXYPad (float x, float y)
{
    const float cutoff = 200.0f * std::pow (100.0f, juce::jlimit (0.0f, 1.0f, x));
    if (auto* p = apvts.getParameter ("cutoff"))
        p->setValueNotifyingHost (p->convertTo0to1 (cutoff));

    const float yy = juce::jlimit (0.0f, 1.0f, y);
    layer0Vol.store (juce::jlimit (0.0f, 1.0f, 1.0f - yy * 0.7f));
    layer1Vol.store (juce::jlimit (0.0f, 1.0f, 0.4f + 0.6f * std::abs (yy - 0.5f) * 2.0f));
    layer2Vol.store (juce::jlimit (0.0f, 1.0f, 0.3f + yy * 0.7f));
}

//==============================================================================
// DARK-ONLY melody generation.
//==============================================================================
void DarkTrapAudioProcessor::generateVariations()
{
    juce::ScopedLock sl (synthLock);
    for (auto& l : layers) l.synth.allNotesOff (0, false);
    variations.clear();

    const int key = selectedKey.load();
    const int mood = selectedMood.load();
    const int genre = selectedGenre.load();

    // ALL DARK SCALES — no major.
    static const int harmonicMinor[]    = { 0, 2, 3, 5, 7, 8, 11 };
    static const int phrygian[]         = { 0, 1, 3, 5, 7, 8, 10 };
    static const int locrian[]          = { 0, 1, 3, 5, 6, 8, 10 };
    static const int phrygianDominant[] = { 0, 1, 4, 5, 7, 8, 10 };

    const int* scale = harmonicMinor;
    switch (mood)
    {
        case 0: scale = harmonicMinor;    break;  // Dark
        case 1: scale = phrygian;         break;  // Haunting
        case 2: scale = locrian;          break;  // Sinister
        case 3: scale = phrygianDominant; break;  // Eerie
        default: scale = harmonicMinor;   break;
    }

    // Genre tempo/density
    double bpm = 140.0;
    float density = 1.0f;
    switch (genre)
    {
        case 0: bpm = 140.0; density = 1.00f; break;  // Trap
        case 1: bpm = 142.0; density = 1.15f; break;  // Drill
        case 2: bpm = 150.0; density = 0.70f; break;  // Plugg
        case 3: bpm = 100.0; density = 0.55f; break;  // Ambient
        default: break;
    }

    const int ticksPer16th = generatedTicksPerQuarterNote / 4;
    const int barTicks = generatedTicksPerQuarterNote * 4;
    const int numBars = 4;
    const int rootNote = 48 + key;

    auto& rng = juce::Random::getSystemRandom();

    // Eerier rhythm patterns (more rests and odd placements)
    static const std::vector<std::vector<int>> melodyRhythms =
    {
        { 0, 3, 7, 10 },             // sparse
        { 0, 2, 5, 8, 11, 14 },      // ghostly
        { 0, 3, 6, 9, 12, 15 },      // syncopated
        { 0, 4, 7, 11 },             // slow
        { 0, 2, 3, 7, 9, 12, 14 },   // trap
        { 0, 5, 10, 15 },            // quarter-strong
        { 0, 2, 6, 10, 13 }          // eerie
    };
    static const std::vector<std::vector<int>> chordRhythms =
    {
        { 0, 4, 8, 12 }, { 0, 3, 8, 11 }, { 0, 6, 10, 14 }, { 0, 2, 8, 10, 14 }
    };
    static const std::vector<std::vector<int>> bassRhythms =
    {
        { 0, 3, 6, 10, 12 }, { 0, 4, 8, 12 }, { 0, 2, 7, 10, 14 }, { 0, 5, 8, 11 }
    };

    for (int v = 0; v < 8; ++v)
    {
        GeneratedVariation variation;
        variation.moodTag = ju
