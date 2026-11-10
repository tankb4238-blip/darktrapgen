#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>
#include <cmath>
#include <map>
#include <algorithm>

static const DarkInstrumentDef darkInstruments[] =
{
    { "Cathedral Bell", "MEL", 7,
      { 1.0, 2.0, 2.4, 3.0, 4.2, 5.4, 6.8, 8.1 },
      { 1.0, 0.6, 0.4, 0.5, 0.3, 0.2, 0.15, 0.1 },
      { 4.0, 3.5, 3.0, 2.5, 2.0, 1.8, 1.5, 1.3 },
      0.02f, 0.05f, 0, 127, 0.005f, 2.0f, 0.55f, 4.0f, 4000.0f },
    { "Ice Bell", "MEL", 5,
      { 1.0, 2.76, 5.40, 8.93, 13.34, 0, 0, 0 },
      { 1.0, 0.6, 0.4, 0.25, 0.15, 0, 0, 0 },
      { 1.5, 1.2, 0.9, 0.6, 0.4, 0, 0, 0 },
      0.0f, 0.0f, 0, 127, 0.001f, 1.0f, 0.30f, 1.5f, 8000.0f },
    { "Ghost Pad", "CHD", 7,
      { 1.0, 1.01, 1.99, 2.01, 3.0, 4.02, 5.0, 0 },
      { 0.7, 0.7, 0.4, 0.4, 0.25, 0.15, 0.1, 0 },
      { 25.0, 25.0, 22.0, 22.0, 20.0, 18.0, 15.0, 0 },
      0.05f, 0.02f, 0, 127, 1.5f, 3.0f, 0.75f, 4.0f, 2500.0f },
    { "Horror Pad", "CHD", 7,
      { 0.5, 0.75, 1.0, 1.33, 1.5, 1.99, 2.0, 0 },
      { 0.5, 0.4, 0.8, 0.3, 0.4, 0.35, 0.3, 0 },
      { 30.0, 28.0, 25.0, 22.0, 20.0, 18.0, 15.0, 0 },
      0.15f, 0.05f, 0, 127, 2.0f, 4.0f, 0.65f, 6.0f, 1500.0f },
    { "Sub 808", "BAS", 3,
      { 1.0, 2.0, 3.0, 0, 0, 0, 0, 0 },
      { 1.0, 0.15, 0.05, 0, 0, 0, 0, 0 },
      { 20.0, 10.0, 5.0, 0, 0, 0, 0, 0 },
      0.0f, 0.15f, 0, 60, 0.005f, 0.5f, 0.90f, 0.8f, 400.0f },
    { "Trap 808", "BAS", 4,
      { 1.0, 2.0, 3.02, 4.0, 0, 0, 0, 0 },
      { 1.0, 0.35, 0.15, 0.08, 0, 0, 0, 0 },
      { 15.0, 9.0, 5.0, 3.5, 0, 0, 0, 0 },
      0.02f, 0.35f, 0, 60, 0.003f, 1.2f, 0.75f, 1.0f, 800.0f },
    { "Eerie Pluck", "MEL", 5,
      { 1.0, 2.76, 5.4, 8.93, 11.7, 0, 0, 0 },
      { 1.0, 0.4, 0.2, 0.1, 0.05, 0, 0, 0 },
      { 0.5, 0.4, 0.3, 0.2, 0.15, 0, 0, 0 },
      0.05f, 0.10f, 0, 127, 0.001f, 0.3f, 0.0f, 0.4f, 6000.0f },
    { "Music Box", "MEL", 5,
      { 1.0, 3.83, 6.98, 9.7, 12.5, 0, 0, 0 },
      { 1.0, 0.5, 0.3, 0.15, 0.08, 0, 0, 0 },
      { 1.2, 0.9, 0.7, 0.5, 0.3, 0, 0, 0 },
      0.0f, 0.0f, 36, 127, 0.001f, 1.5f, 0.25f, 2.0f, 9000.0f },
    { "Tape Piano", "CHD", 6,
      { 1.0, 2.0, 3.0, 4.02, 5.01, 6.0, 0, 0 },
      { 1.0, 0.5, 0.3, 0.15, 0.1, 0.06, 0, 0 },
      { 3.0, 2.0, 1.5, 1.0, 0.8, 0.6, 0, 0 },
      0.03f, 0.0f, 0, 127, 0.002f, 1.5f, 0.35f, 2.5f, 5000.0f },
    { "Metallic Hit", "PRC", 6,
      { 1.0, 2.32, 4.25, 6.63, 8.16, 10.5, 0, 0 },
      { 1.0, 0.6, 0.4, 0.25, 0.15, 0.10, 0, 0 },
      { 1.8, 1.5, 1.2, 0.9, 0.7, 0.5, 0, 0 },
      0.08f, 0.20f, 0, 127, 0.001f, 1.5f, 0.0f, 2.0f, 7000.0f },
    { "Creepy Choir", "CHD", 7,
      { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0 },
      { 1.0, 0.3, 0.7, 0.15, 0.25, 0.08, 0.1, 0 },
      { 25.0, 22.0, 20.0, 18.0, 15.0, 13.0, 11.0, 0 },
      0.12f, 0.0f, 0, 127, 1.0f, 2.0f, 0.70f, 3.0f, 2000.0f },
    { "Dark Strings", "CHD", 8,
      { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 },
      { 1.0, 0.7, 0.5, 0.4, 0.3, 0.25, 0.2, 0.15 },
      { 25.0, 22.0, 20.0, 18.0, 16.0, 14.0, 12.0, 10.0 },
      0.04f, 0.0f, 0, 127, 0.8f, 1.5f, 0.85f, 2.5f, 3000.0f },
    { "Whisper", "PRC", 4,
      { 1.0, 1.98, 3.01, 4.02, 0, 0, 0, 0 },
      { 0.6, 0.4, 0.3, 0.2, 0, 0, 0, 0 },
      { 3.0, 2.5, 2.0, 1.5, 0, 0, 0, 0 },
      0.35f, 0.05f, 0, 127, 0.3f, 1.0f, 0.40f, 2.0f, 2500.0f },
    { "Detuned Saw", "MEL", 8,
      { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 },
      { 1.0, 0.5, 0.33, 0.25, 0.2, 0.17, 0.14, 0.12 },
      { 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0 },
      0.03f, 0.10f, 0, 127, 0.01f, 0.5f, 0.85f, 1.0f, 4000.0f },
    { "Glass", "MEL", 5,
      { 1.0, 2.76, 5.40, 8.93, 13.34, 0, 0, 0 },
      { 1.0, 0.7, 0.5, 0.3, 0.15, 0, 0, 0 },
      { 2.0, 1.5, 1.0, 0.7, 0.5, 0, 0, 0 },
      0.0f, 0.0f, 48, 127, 0.001f, 1.5f, 0.30f, 2.0f, 12000.0f },
    { "Bell Bass", "BAS", 5,
      { 0.5, 1.0, 1.99, 3.0, 4.1, 0, 0, 0 },
      { 0.4, 1.0, 0.5, 0.3, 0.15, 0, 0, 0 },
      { 25.0, 20.0, 18.0, 15.0, 12.0, 0, 0, 0 },
      0.02f, 0.10f, 12, 60, 0.005f, 1.5f, 0.60f, 2.5f, 1500.0f }
};

int DarkTrapAudioProcessor::getNumDarkInstruments()
{
    return (int)(sizeof (darkInstruments) / sizeof (darkInstruments[0]));
}

const DarkInstrumentDef& DarkTrapAudioProcessor::getDarkInstrument (int i)
{
    return darkInstruments[juce::jlimit (0, getNumDarkInstruments() - 1, i)];
}

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

    bool parseSf2Chunks (const juce::uint8* data, size_t size, std::map<juce::String, RiffChunk>& chunks)
    {
        if (size < 12 || std::memcmp (data, "RIFF", 4) != 0 || std::memcmp (data + 8, "sfbk", 4) != 0)
            return false;
        size_t pos = 12;
        while (pos + 8 <= size)
        {
            const juce::uint32 sz = readU32 (data + pos + 4);
            if (pos + 8 + sz > size) break;
            const juce::String id ((const char*) (data + pos), 4);
            if (id == "LIST")
            {
                const juce::String lt ((const char*) (data + pos + 8), 4);
                if (lt == "sdta" || lt == "pdta")
                {
                    size_t sp = pos + 12, end = pos + 8 + sz;
                    while (sp + 8 <= end)
                    {
                        const juce::uint32 ss = readU32 (data + sp + 4);
                        if (sp + 8 + ss > end) break;
                        chunks[juce::String ((const char*) (data + sp), 4)] = { data + sp + 8, ss };
                        sp += 8 + ss + (ss & 1);
                    }
                }
            }
            pos += 8 + sz + (sz & 1);
        }
        return true;
    }

    // FIX C2672: cast both operands to int before jlimit so Type is unambiguous.
    int guessRootNoteFromName (const juce::String& name)
    {
        const auto upper = name.toUpperCase();
        for (int i = 0; i < upper.length(); ++i)
        {
            const auto c = upper[i];
            int semi = (c == 'C') ? 0 : (c == 'D') ? 2 : (c == 'E') ? 4 : (c == 'F') ? 5
                     : (c == 'G') ? 7 : (c == 'A') ? 9 : (c == 'B') ? 11 : -1;
            if (semi >= 0)
            {
                int idx = i + 1;
                if (idx < upper.length() && (upper[idx] == '#' || upper[idx] == 'S')) { ++semi; ++idx; }
                else if (idx < upper.length() && upper[idx] == 'B') { --semi; ++idx; }
                if (idx < upper.length() && juce::CharacterFunctions::isDigit (upper[idx]))
                {
                    const int octaveDigit = static_cast<int> (upper[idx]) - static_cast<int> ('0');
                    const int noteNumber  = 12 * (octaveDigit + 1) + semi;
                    return juce::jlimit (0, 127, noteNumber);
                }
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
        std::unique_ptr<juce::AudioFormatReader> r (fmt.createReaderFor (file));
        if (! r || r->lengthInSamples <= 0) return false;
        const int nCh = juce::jlimit (1, 2, (int) r->numChannels);
        const int nS = (int) juce::jmin (r->lengthInSamples, (juce::int64) std::numeric_limits<int>::max());
        buffer.setSize (nCh, nS);
        r->read (&buffer, 0, nS, 0, true, true);
        sr = r->sampleRate;
        return true;
    }

    bool loadSf2Into (const juce::File& file, juce::Synthesiser& synth)
    {
        juce::MemoryBlock mem;
        file.loadFileAsData (mem);
        const auto* data = static_cast<const juce::uint8*> (mem.getData());
        std::map<juce::String, RiffChunk> chunks;
        if (! parseSf2Chunks (data, mem.getSize(), chunks)) return false;

        for (auto* n : { "smpl", "phdr", "pbag", "pgen", "inst", "ibag", "igen", "shdr" })
            if (chunks.find (n) == chunks.end()) return false;

        const auto* smpl = chunks["smpl"].data; const auto* phdr = chunks["phdr"].data;
        const auto* pbag = chunks["pbag"].data; const auto* pgen = chunks["pgen"].data;
        const auto* inst = chunks["inst"].data; const auto* ibag = chunks["ibag"].data;
        const auto* igen = chunks["igen"].data; const auto* shdr = chunks["shdr"].data;

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
            const int pbS = readU16 (phdr + pi * 38 + 24);
            const int pbE = readU16 (phdr + (pi + 1) * 38 + 24);
            for (int pbi = pbS; pbi < pbE && pbi < pbagCount - 1; ++pbi)
            {
                const int gS = readU16 (pbag + pbi * 4);
                const int gE = readU16 (pbag + (pbi + 1) * 4);
                int instIdx = -1;
                for (int gi = gS; gi < gE && gi < pgenCount - 1; ++gi)
                    if (readU16 (pgen + gi * 4) == GEN_INSTRUMENT)
                    { instIdx = readU16 (pgen + gi * 4 + 2); break; }
                if (instIdx < 0 || instIdx + 1 >= instCount) continue;

                const int ibS = readU16 (inst + instIdx * 22 + 20);
                const int ibE = readU16 (inst + (instIdx + 1) * 22 + 20);
                for (int ibi = ibS; ibi < ibE && ibi < ibagCount - 1; ++ibi)
                {
                    const int igS = readU16 (ibag + ibi * 4);
                    const int igE = readU16 (ibag + (ibi + 1) * 4);
                    int sampIdx = -1, rootKey = -1, kLo = 0, kHi = 127;
                    int sampModes = 0, cT = 0, fT = 0, att = 0;
                    for (int gi = igS; gi < igE && gi < igenCount - 1; ++gi)
                    {
                        const int op = readU16 (igen + gi * 4);
                        const int am = readU16 (igen + gi * 4 + 2);
                        switch (op)
                        {
                            case GEN_SAMPLE_ID: sampIdx = am; break;
                            case GEN_KEY_RANGE: kLo = am & 0xFF; kHi = (am >> 8) & 0xFF; break;
                            case GEN_OVERRIDING_ROOT_KEY: rootKey = am; break;
                            case GEN_SAMPLE_MODES: sampModes = am; break;
                            case GEN_COARSE_TUNE: cT = (juce::int16) am; break;
                            case GEN_FINE_TUNE: fT = (juce::int16) am; break;
                            case GEN_INITIAL_ATTENUATION: att = am; break;
                            default: break;
                        }
                    }
                    if (sampIdx < 0 || sampIdx + 1 >= shdrCount) continue;
                    const auto* sh = shdr + sampIdx * 46;
                    const auto sS = readU32 (sh + 20), sE = readU32 (sh + 24);
                    const auto lS = readU32 (sh + 28), lE = readU32 (sh + 32);
                    const auto sR = readU32 (sh + 36);
                    const auto origP = (int) sh[40], pCorr = (int)(juce::int8) sh[41];
                    if (sE <= sS || sE > (juce::uint32) smplFrames) continue;
                    const int nF = (int)(sE - sS);
                    if (nF <= 0) continue;
                    auto* s = new UniversalSound ("SF2");
                    s->sampleBuffer.setSize (1, nF);
                    for (int i = 0; i < nF; ++i)
                        s->sampleBuffer.setSample (0, i, (float) readI16 (smpl + (sS + (juce::uint32) i) * 2) / 32768.0f);
                    s->sourceSampleRate = sR > 0 ? (double) sR : 44100.0;
                    s->rootNote = (rootKey >= 0 && rootKey <= 127) ? rootKey : juce::jlimit (0, 127, origP);
                    s->lowNote = juce::jlimit (0, 127, kLo);
                    s->highNote = juce::jlimit (0, 127, kHi);
                    if (s->lowNote > s->highNote) std::swap (s->lowNote, s->highNote);
                    s->loopEnabled = (sampModes & 1) != 0;
                    if (s->loopEnabled)
                    {
                        const int ls = (int)(lS - sS), le = (int)(lE - sS);
                        if (ls >= 0 && le > ls && le <= nF) { s->loopStart = ls; s->loopEnd = le; }
                        else { s->loopStart = 0; s->loopEnd = nF; }
                    }
                    s->tuneCents = (float)(pCorr + fT + cT * 100);
                    s->gain = juce::Decibels::decibelsToGain ((float) att * -0.1f);
                    synth.addSound (s);
                    addedAny = true;
                }
            }
        }
        return addedAny;
    }

    bool loadSfzInto (const juce::File& file, juce::Synthesiser& synth, juce::AudioFormatManager& fmt)
    {
        const auto text = file.loadFileAsString();
        if (text.isEmpty()) return false;
        juce::StringArray lines;
        lines.addLines (text);
        std::map<juce::String, juce::String> region;
        bool addedAny = false;

        auto flush = [&]()
        {
            if (region.empty()) return;
            const auto sp = region["sample"];
            if (sp.isEmpty()) { region.clear(); return; }
            const auto sf = file.getParentDirectory().getChildFile (sp.unquoted());
            if (! sf.existsAsFile()) { region.clear(); return; }
            juce::AudioBuffer<float> buf;
            double sr = 44100.0;
            if (! loadAudioFileToBuffer (fmt, sf, buf, sr)) { region.clear(); return; }
            auto* s = new UniversalSound (sf.getFileNameWithoutExtension());
            s->sampleBuffer = std::move (buf);
            s->sourceSampleRate = sr;
            const auto rt = region.count ("pitch_keycenter") ? region["pitch_keycenter"]
                          : region.count ("key") ? region["key"] : juce::String ("60");
            s->rootNote = guessRootNoteFromName (rt);
            s->lowNote = juce::jlimit (0, 127, guessRootNoteFromName (region.count ("lokey") ? region["lokey"] : rt));
            s->highNote = juce::jlimit (0, 127, guessRootNoteFromName (region.count ("hikey") ? region["hikey"] : rt));
            if (s->lowNote > s->highNote) std::swap (s->lowNote, s->highNote);
            s->loopEnabled = region.count ("loop_mode") && region["loop_mode"].contains ("loop");
            if (s->loopEnabled)
            {
                const int n = s->sampleBuffer.getNumSamples();
                s->loopStart = region.count ("loop_start") ? region["loop_start"].getIntValue() : 0;
                s->loopEnd = region.count ("loop_end") ? region["loop_end"].getIntValue() : n;
            }
            if (region.count ("volume"))
                s->gain = juce::Decibels::decibelsToGain (region["volume"].getFloatValue());
            synth.addSound (s);
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
                const int c = line.indexOfChar ('>');
                if (c >= 0) line = line.substring (c + 1).trim();
                if (line.isEmpty()) continue;
            }
            juce::StringArray tk;
            tk.addTokens (line, " \t", "\"");
            for (const auto& t : tk)
            {
                const int eq = t.indexOfChar ('=');
                if (eq > 0)
                    region[t.substring (0, eq).trim().toLowerCase()] = t.substring (eq + 1).trim().unquoted();
            }
        }
        flush();
        return addedAny;
    }
}

UniversalVoice::UniversalVoice (VoiceParams& paramsIn) : params (paramsIn) {}

void UniversalVoice::startNote (int note, float vel, juce::SynthesiserSound* snd, int pw)
{
    currentSound = dynamic_cast<UniversalSound*> (snd);
    if (! currentSound) return;
    midiNote = note;
    velocityGain = 0.3f + 0.7f * vel;
    samplePos = 0.0;
    modalTime = 0.0;
    modalActive = false;
    pitchWheelMoved (pw);

    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    if (lastSampleRate != sr) { adsr.setSampleRate (sr); updateFilters (sr); lastSampleRate = sr; }

    const bool cu = currentSound->useCustomEnvelope;
    const float a = cu ? currentSound->presetAttack  : (params.attack  ? params.attack->load()  : 0.01f);
    const float d = cu ? currentSound->presetDecay   : (params.decay   ? params.decay->load()   : 0.2f);
    const float s = cu ? currentSound->presetSustain : (params.sustain ? params.sustain->load() : 0.8f);
    const float r = cu ? currentSound->presetRelease : (params.release ? params.release->load() : 0.3f);
    adsr.setParameters ({ a, d, s, r });
    adsr.noteOn();

    if (currentSound->isModal || currentSound->sampleBuffer.getNumSamples() == 0)
    {
        initModal (note);
        modalActive = true;
        return;
    }

    const double semis = (double)(note - currentSound->rootNote) + (double) currentSound->tuneCents / 100.0;
    pitchRatio = std::pow (2.0, semis / 12.0) * (currentSound->sourceSampleRate / sr);
}

void UniversalVoice::stopNote (float, bool tail)
{
    if (tail) adsr.noteOff();
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
    const double base = juce::MidiMessage::getMidiNoteInHertz (noteNumber);
    numActiveModes = currentSound->numPartials > 0
                     ? juce::jlimit (1, maxModes, currentSound->numPartials) : 6;

    double sum = 0.0;
    for (int i = 0; i < numActiveModes; ++i)
    {
        const double r = currentSound->numPartials > 0 ? currentSound->partialRatios[i] : (1.0 + i * 1.05);
        const double a = currentSound->numPartials > 0 ? currentSound->partialAmps[i]   : (1.0 / (i + 1.0));
        const double d = currentSound->numPartials > 0 ? currentSound->partialDecays[i] : (1.4 + i * 1.1);
        modalFreq[i] = base * r;
        modalPhase[i] = juce::MathConstants<double>::twoPi * juce::Random::getSystemRandom().nextDouble();
        modalAmp[i] = a;
        modalDecay[i] = d;
        sum += a;
    }
    if (sum > 0.0)
        for (int i = 0; i < numActiveModes; ++i)
            modalAmp[i] /= sum;
}

void UniversalVoice::updateFilters (double sr)
{
    const float c = (currentSound && currentSound->useCustomEnvelope)
                    ? currentSound->presetCutoff
                    : (params.cutoff ? params.cutoff->load() : 8000.0f);
    const auto co = juce::IIRCoefficients::makeLowPass (sr, juce::jlimit (20.0, sr * 0.49, (double) c), 0.7071);
    filterL.setCoefficients (co);
    filterR.setCoefficients (co);
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

void UniversalVoice::renderNextBlock (juce::AudioBuffer<float>& out, int ss, int nS)
{
    if (! currentSound || nS <= 0) return;
    const double sr = getSampleRate();
    if (sr <= 0.0) return;
    if (lastSampleRate != sr) { adsr.setSampleRate (sr); updateFilters (sr); lastSampleRate = sr; }

    const bool cu = currentSound->useCustomEnvelope;
    const float a = cu ? currentSound->presetAttack  : (params.attack  ? params.attack->load()  : 0.01f);
    const float d = cu ? currentSound->presetDecay   : (params.decay   ? params.decay->load()   : 0.2f);
    const float s = cu ? currentSound->presetSustain : (params.sustain ? params.sustain->load() : 0.8f);
    const float r = cu ? currentSound->presetRelease : (params.release ? params.release->load() : 0.3f);
    adsr.setParameters ({ a, d, s, r });
    updateFilters (sr);

    const int nCh = out.getNumChannels();

    for (int i = 0; i < nS; ++i)
    {
        if (! adsr.isActive()) { clearCurrentNote(); break; }
        const float env = adsr.getNextSample();
        float L = 0.0f, R = 0.0f;

        if (modalActive)
        {
            modalTime += 1.0 / sr;
            double m = 0.0;
            for (int k = 0; k < numActiveModes; ++k)
            {
                const double md = std::exp (-modalDecay[k] * modalTime);
                m += modalAmp[k] * std::sin (modalPhase[k]) * md;
                modalPhase[k] += juce::MathConstants<double>::twoPi * modalFreq[k] / sr;
            }
            if (currentSound->noiseAmount > 0.0f)
                m += (juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0)
                     * currentSound->noiseAmount * 0.5;
            if (currentSound->driveAmount > 0.0f)
            {
                const double dr = 1.0 + currentSound->driveAmount * 8.0;
                m = std::tanh (m * dr) / dr;
            }
            L = R = (float) m;
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
            if (! currentSound->loopEnabled && samplePos >= (double) n) { clearCurrentNote(); break; }
            L = readSample (0, samplePos);
            R = currentSound->sampleBuffer.getNumChannels() > 1 ? readSample (1, samplePos) : L;
            samplePos += pitchRatio;
        }

        L = filterL.processSingleSampleRaw (L);
        R = filterR.processSingleSampleRaw (R);
        const float g = env * velocityGain * currentSound->gain;

        for (int ch = 0; ch < nCh; ++ch)
            out.addSample (ch, ss + i, (ch == 1 ? R : L) * g);
    }
}

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

    loadDarkInstrumentPreset (0, 0);
    loadDarkInstrumentPreset (1, 2);
    loadDarkInstrumentPreset (2, 4);
}

juce::AudioProcessorValueTreeState::ParameterLayout
DarkTrapAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "cutoff", 1 }, "Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 8000.0f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.4f), 0.01f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.4f), 0.2f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.4f), 0.3f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "reverb", 1 }, "Reverb",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "delay", 1 }, "Delay",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.20f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "chorus", 1 }, "Chorus",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "distortion", 1 }, "Distortion",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "flanger", 1 }, "Flanger",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    L.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "halfSpeed", 1 }, "Half-Speed", false));
    return L;
}

void DarkTrapAudioProcessor::setupLayer (int index)
{
    auto& l = layers[(size_t) index];
    l.synth.setCurrentPlaybackSampleRate (44100.0);
    l.synth.clearVoices();
    l.synth.clearSounds();
    for (int v = 0; v < 12; ++v)
        l.synth.addVoice (new UniversalVoice (voiceParams));
    l.synth.setNoteStealingEnabled (true);
}

void DarkTrapAudioProcessor::loadDarkInstrumentPreset (int li, int pi)
{
    if (li < 0 || li > 2) return;
    pi = juce::jlimit (0, getNumDarkInstruments() - 1, pi);
    juce::ScopedLock sl (synthLock);
    auto& layer = layers[(size_t) li];
    const auto& def = darkInstruments[pi];

    layer.synth.allNotesOff (0, false);
    layer.synth.clearSounds();

    auto* s = new UniversalSound (def.name);
    s->isModal = true;
    s->numPartials = def.numPartials;
    for (int i = 0; i < 8; ++i)
    {
        s->partialRatios[i] = def.ratios[i];
        s->partialAmps[i]   = def.amps[i];
        s->partialDecays[i] = def.decays[i];
    }
    s->noiseAmount = def.noiseAmount;
    s->driveAmount = def.driveAmount;
    s->lowNote = def.lowNote;
    s->highNote = def.highNote;
    s->gain = 1.0f;
    s->useCustomEnvelope = true;
    s->presetAttack  = def.attack;
    s->presetDecay   = def.decay;
    s->presetSustain = def.sustain;
    s->presetRelease = def.release;
    s->presetCutoff  = def.cutoff;
    layer.synth.addSound (s);
    layer.sourceName = def.name;
    layer.presetIndex = pi;
}

void DarkTrapAudioProcessor::loadSoundIntoLayer (int li, const juce::Array<juce::File>& files)
{
    if (files.isEmpty() || li < 0 || li > 2) return;
    juce::ScopedLock sl (synthLock);
    auto& layer = layers[(size_t) li];
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
        for (const auto& f : files)
        {
            juce::AudioBuffer<float> buf;
            double sr = 44100.0;
            if (! loadAudioFileToBuffer (formatManager, f, buf, sr)) continue;
            auto* s = new UniversalSound (f.getFileNameWithoutExtension());
            s->sampleBuffer = std::move (buf);
            s->sourceSampleRate = sr;
            s->rootNote = guessRootNoteFromName (f.getFileNameWithoutExtension());
            if (multi)
            {
                s->lowNote  = juce::jlimit (0, 127, s->rootNote - 3);
                s->highNote = juce::jlimit (0, 127, s->rootNote + 3);
            }
            layer.synth.addSound (s);
            loaded = true;
        }
    }
    if (loaded) layer.sourceName = first.getFileNameWithoutExtension();
    else loadDarkInstrumentPreset (li, 0);
}

void DarkTrapAudioProcessor::prepareToPlay (double sr, int)
{
    juce::ScopedLock sl (synthLock);
    for (auto& l : layers) l.synth.setCurrentPlaybackSampleRate (sr);

    delayBuffer.setSize (2, juce::jmax (1, (int)(2.0 * sr)));
    delayBuffer.clear();
    delayWritePos = 0;
    chorusBuffer.setSize (2, juce::jmax (1, (int)(0.05 * sr)));
    chorusBuffer.clear();
    chorusWritePos = 0;
    chorusPhase = 0.0f;
    flangerBuffer.setSize (2, juce::jmax (1, (int)(0.02 * sr)));
    flangerBuffer.clear();
    flangerWritePos = 0;
    flangerPhase = 0.0f;
    halfSpeedBuffer.setSize (2, juce::jmax (1, (int)(3.0 * sr)));
    halfSpeedBuffer.clear();
    halfSpeedWritePos = 0;
    halfSpeedReadPos = 0.0;
    reverb.reset();
    generatedPlaybackTime = 0.0;
}

bool DarkTrapAudioProcessor::isBusesLayoutSupported (const BusesLayout& L) const
{
    const auto o = L.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo();
}

void DarkTrapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    juce::ScopedLock sl (synthLock);
    const int nS = buffer.getNumSamples();
    const double sr = getSampleRate();
    if (nS <= 0 || sr <= 0.0) return;
    buffer.clear();

    const auto* v = (! variations.empty() && currentVariationIndex < (int) variations.size())
                    ? &variations[(size_t) currentVariationIndex] : nullptr;

    juce::AudioBuffer<float> lb (2, nS);
    const float vols[3] = { layer0Vol.load(), layer1Vol.load(), layer2Vol.load() };

    for (int li = 0; li < 3; ++li)
    {
        auto& layer = layers[(size_t) li];
        if (layer.muted || layer.volume < 0.001f) continue;
        lb.clear();

        juce::MidiBuffer lm;
        for (const auto md : midi)
            lm.addEvent (md.getMessage(), md.samplePosition);

        if (auditioning && v)
        {
            const auto& seq = (li == 0) ? v->melodySeq : (li == 1) ? v->chordSeq : v->bassSeq;
            const double spt = (60.0 / v->bpm) / (double) generatedTicksPerQuarterNote;
            const double bS = generatedPlaybackTime;
            const double bE = bS + (double) nS / sr;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                const auto* ev = seq.getEventPointer (i);
                if (! ev || ! ev->message.isNoteOnOrOff()) continue;
                const double t = seq.getEventTime (i) * spt;
                if (t >= bS && t < bE)
                    lm.addEvent (ev->message, juce::jlimit (0, nS - 1, (int)((t - bS) * sr)));
            }
        }

        layer.synth.renderNextBlock (lb, lm, 0, nS);
        const float g = layer.volume * vols[li];
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* src = lb.getReadPointer (juce::jmin (ch, 1));
            float* dst = buffer.getWritePointer (ch);
            for (int i = 0; i < nS; ++i) dst[i] += src[i] * g;
        }
    }

    if (auditioning && v)
    {
        generatedPlaybackTime += (double) nS / sr;
        const double total = (60.0 / v->bpm) * v->bars * 4.0;
        if (total > 0.0)
            while (generatedPlaybackTime >= total)
                generatedPlaybackTime -= total;
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
            if (chorusPhase > juce::MathConstants<float>::twoPi)
                chorusPhase -= juce::MathConstants<float>::twoPi;
            const float ms = 15.0f + 5.0f * std::sin (chorusPhase);
            const int ds = juce::jlimit (1, mx - 1, (int)(ms * 0.001f * (float) sr));
            int rp = chorusWritePos - ds;
            while (rp < 0) rp += mx;
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
            if (flangerPhase > juce::MathConstants<float>::twoPi)
                flangerPhase -= juce::MathConstants<float>::twoPi;
            const float ms = 3.0f + 2.0f * std::sin (flangerPhase);
            const int ds = juce::jlimit (1, mx - 1, (int)(ms * 0.001f * (float) sr));
            int rp = flangerWritePos - ds;
            while (rp < 0) rp += mx;
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
            int rp = delayWritePos - ds;
            if (rp < 0) rp += mx;
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
                const float aa = halfSpeedBuffer.getSample (ch, i0);
                const float bb = halfSpeedBuffer.getSample (ch, i1);
                buffer.setSample (ch, i, aa * (1.0f - f) + bb * f);
            }
            halfSpeedReadPos += 0.5;
            if (halfSpeedReadPos >= mx) halfSpeedReadPos -= mx;
        }
    }
}

void DarkTrapAudioProcessor::setXYPad (float x, float y)
{
    const float cut = 200.0f * std::pow (100.0f, juce::jlimit (0.0f, 1.0f, x));
    if (auto* p = apvts.getParameter ("cutoff"))
        p->setValueNotifyingHost (p->convertTo0to1 (cut));
    const float yy = juce::jlimit (0.0f, 1.0f, y);
    layer0Vol.store (juce::jlimit (0.0f, 1.0f, 1.0f - yy * 0.7f));
    layer1Vol.store (juce::jlimit (0.0f, 1.0f, 0.4f + 0.6f * std::abs (yy - 0.5f) * 2.0f));
    layer2Vol.store (juce::jlimit (0.0f, 1.0f, 0.3f + yy * 0.7f));
}

void DarkTrapAudioProcessor::generateVariations()
{
    juce::ScopedLock synthScope (synthLock);
    for (auto& l : layers) l.synth.allNotesOff (0, false);
    variations.clear();

    const int key = selectedKey.load();
    const int mood = selectedMood.load();
    const int genre = selectedGenre.load();

    static const int harmonicMinor[]    = { 0, 2, 3, 5, 7, 8, 11 };
    static const int phrygian[]         = { 0, 1, 3, 5, 7, 8, 10 };
    static const int locrian[]          = { 0, 1, 3, 5, 6, 8, 10 };
    static const int phrygianDominant[] = { 0, 1, 4, 5, 7, 8, 10 };

    const int* scale = harmonicMinor;
    switch (mood)
    {
        case 0: scale = harmonicMinor;    break;
        case 1: scale = phrygian;         break;
        case 2: scale = locrian;          break;
        case 3: scale = phrygianDominant; break;
        default: break;
    }

    double bpm = 140.0;
    float density = 1.0f;
    switch (genre)
    {
        case 0: bpm = 140.0; density = 1.00f; break;
        case 1: bpm = 142.0; density = 1.15f; break;
        case 2: bpm = 150.0; density = 0.70f; break;
        case 3: bpm = 100.0; density = 0.55f; break;
        default: break;
    }

    const int t16 = generatedTicksPerQuarterNote / 4;
    const int barTicks = generatedTicksPerQuarterNote * 4;
    const int numBars = 4;
    const int rootNote = 48 + key;
    auto& rng = juce::Random::getSystemRandom();

    static const std::vector<std::vector<int>> mRhythms = {
        { 0,3,7,10 }, { 0,2,5,8,11,14 }, { 0,3,6,9,12,15 }, { 0,4,7,11 },
        { 0,2,3,7,9,12,14 }, { 0,5,10,15 }, { 0,2,6,10,13 }
    };
    static const std::vector<std::vector<int>> cRhythms = {
        { 0,4,8,12 }, { 0,3,8,11 }, { 0,6,10,14 }, { 0,2,8,10,14 }
    };
    static const std::vector<std::vector<int>> bRhythms = {
        { 0,3,6,10,12 }, { 0,4,8,12 }, { 0,2,7,10,14 }, { 0,5,8,11 }
    };

    for (int v = 0; v < 8; ++v)
    {
        GeneratedVariation var;
        var.moodTag = juce::StringArray { "Dark", "Haunting", "Sinister", "Eerie" }[mood];
        var.genreTag = juce::StringArray { "Trap", "Drill", "Plugg", "Ambient" }[genre];
        var.bpm = bpm;
        var.rootNote = rootNote;
        var.bars = numBars;

        {
            int deg = 4 + rng.nextInt (3);
            for (int b = 0; b < numBars; ++b)
            {
                const auto& rh = mRhythms[(size_t)((v + b) % (int) mRhythms.size())];
                for (int step : rh)
                {
                    if (rng.nextFloat() > density * 0.75f) continue;
                    const int tick = b * barTicks + step * t16;
                    int move = (rng.nextFloat() < 0.7f) ? -(rng.nextInt (3)) : rng.nextInt (2);
                    if (rng.nextFloat() < 0.15f) move = rng.nextBool() ? -4 : 3;
                    deg = juce::jlimit (0, 13, deg + move);
                    const int oct = deg / 7;
                    const int d = deg % 7;
                    int note = rootNote + 12 + scale[d] + oct * 12;
                    if (rng.nextFloat() < 0.20f)
                    {
                        const int dd = (d == 0) ? 1 : (d == 2) ? 4 : 5;
                        note = rootNote + 12 + scale[dd] + oct * 12;
                    }
                    if (rng.nextFloat() < 0.15f) note -= 1;
                    if (rng.nextFloat() < 0.06f) note -= 6;
                    note = juce::jlimit (36, 84, note);
                    float vel = 0.55f + 0.35f * rng.nextFloat();
                    if (step == 0) vel = juce::jmax (vel, 0.88f);
                    if (rng.nextFloat() < 0.15f) vel *= 0.6f;
                    const auto mv = (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f));
                    const int len = rng.nextFloat() < 0.30f ? t16 * 2 : t16;
                    var.melodySeq.addEvent (juce::MidiMessage::noteOn (1, note, mv), tick);
                    var.melodySeq.addEvent (juce::MidiMessage::noteOff (1, note), tick + len);
                    if (rng.nextFloat() < 0.10f)
                    {
                        const int rn = juce::jlimit (36, 84, note + (rng.nextBool() ? 12 : -12));
                        const int rt = tick + t16 / 2;
                        const auto rv = (juce::uint8) juce::jlimit (1, 127, (int)(mv * 0.7f));
                        var.melodySeq.addEvent (juce::MidiMessage::noteOn (1, rn, rv), rt);
                        var.melodySeq.addEvent (juce::MidiMessage::noteOff (1, rn), rt + t16 / 2);
                    }
                }
            }
        }

        {
            int deg = rng.nextInt (7);
            for (int b = 0; b < numBars; ++b)
            {
                const auto& rh = cRhythms[(size_t)((v + b) % (int) cRhythms.size())];
                for (int step : rh)
                {
                    if (rng.nextFloat() > density * 0.75f) continue;
                    const int tick = b * barTicks + step * t16;
                    deg = juce::jlimit (0, 6, deg + (rng.nextInt (3) - 1));
                    const int r = rootNote + scale[deg];
                    const int th = rootNote + scale[(deg + 2) % 7] + (deg + 2 >= 7 ? 12 : 0);
                    const int fi = rootNote + scale[(deg + 4) % 7] + (deg + 4 >= 7 ? 12 : 0);
                    const float vel = 0.45f + 0.30f * rng.nextFloat();
                    const auto mv = (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f));
                    const int len = t16 * 3;
                    for (int n : { r, th, fi })
                    {
                        n = juce::jlimit (36, 72, n);
                        var.chordSeq.addEvent (juce::MidiMessage::noteOn (1, n, mv), tick);
                        var.chordSeq.addEvent (juce::MidiMessage::noteOff (1, n), tick + len);
                    }
                }
            }
        }

        {
            int deg = rng.nextInt (3);
            for (int b = 0; b < numBars; ++b)
            {
                const auto& rh = bRhythms[(size_t)((v + b) % (int) bRhythms.size())];
                for (int step : rh)
                {
                    const int tick = b * barTicks + step * t16;
                    deg = juce::jlimit (0, 6, deg + (rng.nextInt (3) - 1));
                    int note = rootNote - 12 + scale[deg];
                    note = juce::jlimit (24, 48, note);
                    const float vel = 0.6f + 0.4f * rng.nextFloat();
                    const auto mv = (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f));
                    var.bassSeq.addEvent (juce::MidiMessage::noteOn (1, note, mv), tick);
                    var.bassSeq.addEvent (juce::MidiMessage::noteOff (1, note), tick + t16);
                    if (genre == 1 && rng.nextFloat() < 0.15f)
                    {
                        const int slideNote = juce::jlimit (24, 48, note - 1);
                        var.bassSeq.addEvent (juce::MidiMessage::noteOn (1, slideNote, mv), tick + t16);
                        var.bassSeq.addEvent (juce::MidiMessage::noteOff (1, slideNote), tick + t16 + t16 / 2);
                    }
                }
            }
        }

        const int tempoMeta = (int)(60000000.0 / bpm);
        var.melodySeq.addEvent (juce::MidiMessage::tempoMetaEvent (tempoMeta), 0);
        var.chordSeq.addEvent  (juce::MidiMessage::tempoMetaEvent (tempoMeta), 0);
        var.bassSeq.addEvent   (juce::MidiMessage::tempoMetaEvent (tempoMeta), 0);
        var.melodySeq.addEvent (juce::MidiMessage::endOfTrack(), numBars * barTicks);
        var.chordSeq.addEvent  (juce::MidiMessage::endOfTrack(), numBars * barTicks);
        var.bassSeq.addEvent   (juce::MidiMessage::endOfTrack(), numBars * barTicks);
        var.melodySeq.updateMatchedPairs();
        var.chordSeq.updateMatchedPairs();
        var.bassSeq.updateMatchedPairs();

        variations.push_back (std::move (var));
    }

    currentVariationIndex = 0;
    generatedPlaybackTime = 0.0;
    generatedLengthSeconds = (60.0 / bpm) * numBars * 4.0;
    auditioning = true;
}

void DarkTrapAudioProcessor::setActiveVariation (int index)
{
    juce::ScopedLock sl (synthLock);
    if (index >= 0 && index < (int) variations.size())
    {
        currentVariationIndex = index;
        generatedPlaybackTime = 0.0;
        auditioning = true;
    }
}

juce::File DarkTrapAudioProcessor::getVariationMidiFile (int index)
{
    if (variations.empty()) generateVariations();
    index = juce::jlimit (0, (int) variations.size() - 1, index);
    juce::ScopedLock sl (synthLock);
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("DarkTrap_" + juce::String (index) + ".mid");
    file.deleteFile();
    juce::MidiFile midi;
    midi.setTicksPerQuarterNote (generatedTicksPerQuarterNote);
    midi.addTrack (variations[(size_t) index].melodySeq);
    midi.addTrack (variations[(size_t) index].chordSeq);
    midi.addTrack (variations[(size_t) index].bassSeq);
    if (auto stream = file.createOutputStream()) { midi.writeTo (*stream); stream->flush(); }
    return file;
}

void DarkTrapAudioProcessor::exportAudioToFile (const juce::File& out)
{
    if (variations.empty()) return;
    juce::ScopedLock sl (synthLock);
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    const auto& v = variations[(size_t) currentVariationIndex];
    const int total = (int)((60.0 / v.bpm) * v.bars * 4.0 * sr) + (int)(sr * 3.0);
    const double spt = (60.0 / v.bpm) / (double) generatedTicksPerQuarterNote;

    juce::AudioBuffer<float> render (2, total);
    render.clear();

    for (int li = 0; li < 3; ++li)
    {
        const auto& seq = (li == 0) ? v.melodySeq : (li == 1) ? v.chordSeq : v.bassSeq;
        juce::AudioBuffer<float> layerBuf (2, total);
        layerBuf.clear();
        const int bs = 512;
        for (int off = 0; off < total; off += bs)
        {
            const int n = juce::jmin (bs, total - off);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            juce::MidiBuffer midi;
            const double t0 = (double) off / sr;
            const double t1 = t0 + (double) n / sr;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                const auto* ev = seq.getEventPointer (i);
                if (! ev || ! ev->message.isNoteOnOrOff()) continue;
                const double t = seq.getEventTime (i) * spt;
                if (t >= t0 && t < t1)
                    midi.addEvent (ev->message, juce::jlimit (0, n - 1, (int)((t - t0) * sr)));
            }
            layers[(size_t) li].synth.renderNextBlock (blk, midi, 0, n);
            for (int ch = 0; ch < 2; ++ch)
                layerBuf.copyFrom (ch, off, blk, ch, 0, n);
        }
        const float g = layers[(size_t) li].volume;
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* src = layerBuf.getReadPointer (ch);
            float* dst = render.getWritePointer (ch);
            for (int i = 0; i < total; ++i) dst[i] += src[i] * g;
        }
    }

    out.deleteFile();
    if (auto stream = out.createOutputStream())
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (stream.release(), sr, 2, 24, {}, 0));
        if (w) { w->writeFromAudioSampleBuffer (render, 0, total); w->flush(); }
    }
}

void DarkTrapAudioProcessor::exportLayerStems (const juce::File& folder)
{
    if (variations.empty()) return;
    juce::ScopedLock sl (synthLock);
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    const auto& v = variations[(size_t) currentVariationIndex];
    const int total = (int)((60.0 / v.bpm) * v.bars * 4.0 * sr) + (int)(sr * 2.0);
    const double spt = (60.0 / v.bpm) / (double) generatedTicksPerQuarterNote;
    const char* names[3] = { "Melody", "Chords", "Bass" };

    for (int li = 0; li < 3; ++li)
    {
        const auto& seq = (li == 0) ? v.melodySeq : (li == 1) ? v.chordSeq : v.bassSeq;
        juce::AudioBuffer<float> stem (2, total);
        stem.clear();
        const int bs = 512;
        for (int off = 0; off < total; off += bs)
        {
            const int n = juce::jmin (bs, total - off);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            juce::MidiBuffer midi;
            const double t0 = (double) off / sr;
            const double t1 = t0 + (double) n / sr;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                const auto* ev = seq.getEventPointer (i);
                if (! ev || ! ev->message.isNoteOnOrOff()) continue;
                const double t = seq.getEventTime (i) * spt;
                if (t >= t0 && t < t1)
                    midi.addEvent (ev->message, juce::jlimit (0, n - 1, (int)((t - t0) * sr)));
            }
            layers[(size_t) li].synth.renderNextBlock (blk, midi, 0, n);
            for (int ch = 0; ch < 2; ++ch)
                stem.copyFrom (ch, off, blk, ch, 0, n);
        }
        const auto file = folder.getChildFile ("Stem_" + juce::String (li + 1) + "_" + names[li] + ".wav");
        file.deleteFile();
        if (auto stream = file.createOutputStream())
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (stream.release(), sr, 2, 24, {}, 0));
            if (w) { w->writeFromAudioSampleBuffer (stem, 0, total); w->flush(); }
        }
    }
}

void DarkTrapAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DarkTrapAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DarkTrapAudioProcessor::createEditor()
{
    return new DarkTrapAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkTrapAudioProcessor();
}
