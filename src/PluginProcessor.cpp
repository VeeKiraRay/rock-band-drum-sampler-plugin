#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RBDrumSamplerData.h"

juce::AudioProcessorValueTreeState::ParameterLayout RBDrumSamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (auto& d : kDrums)
    {
        juce::String p = d.prefix;
        juce::String lbl = d.label;
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            p + "_vol", lbl + " Volume",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            p + "_atk", lbl + " Attack",
            juce::NormalisableRange<float>(0.0f, 0.5f, 0.001f), 0.001f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            p + "_rel", lbl + " Release",
            juce::NormalisableRange<float>(0.01f, 3.0f, 0.01f), d.defaultRelease));
    }
    return { params.begin(), params.end() };
}

RBDrumSamplerAudioProcessor::RBDrumSamplerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "State", createParameterLayout())
{
    activeTarget.fill(-1);
    formatManager.registerBasicFormats();
    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

RBDrumSamplerAudioProcessor::~RBDrumSamplerAudioProcessor() {}

void RBDrumSamplerAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    loadSamples();
}

void RBDrumSamplerAudioProcessor::releaseResources() {}

bool RBDrumSamplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void RBDrumSamplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    if (muted.load())
        return;

    // Update each sample's envelope from current parameter values
    for (int i = 0; i < (int)kDrums.size(); ++i)
    {
        if (sounds[i])
        {
            juce::String p = kDrums[i].prefix;
            sounds[i]->setEnvelopeParameters({
                apvts.getRawParameterValue(p + "_atk")->load(),
                0.0f,  // decay (unused — sustain is 1.0)
                1.0f,  // sustain
                apvts.getRawParameterValue(p + "_rel")->load()
            });
        }
    }

    // First pass: record which RB pitches are present this block
    bool has[128] = {};
    for (auto meta : midiMessages)
        if (meta.getMessage().isNoteOn())
            has[meta.getMessage().getNoteNumber()] = true;

    // Y+G combo: Yellow (98) + Green (100) both cymbal (no tom markers) = two crashes
    const bool yg_combo = has[98] && has[100] && !has[110] && !has[112];

    // Second pass: remap RB pitches to internal sampler notes
    juce::MidiBuffer remapped;
    for (auto meta : midiMessages)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            int target = -1;  // -1 = drop
            switch (msg.getNoteNumber())
            {
                case 96: target = 96;  break;  // Kick
                case 97: target = 97;  break;  // Snare
                case 98:
                    if      (has[110]) target = 110;  // Yellow tom marker present
                    else if (yg_combo) target = 101;  // Y+G combo: play Crsh2
                    else               target = 98;   // Normal hi-hat
                    break;
                case 99:
                    target = has[111] ? 111 : 99;     // Blue tom or ride
                    break;
                case 100:
                    target = has[112] ? 112 : 100;    // Green tom or Crsh1
                    break;
                // 110/111/112 are markers only — no sound, drop them
                default: break;
            }

            if (target >= 0)
            {
                activeTarget[msg.getNoteNumber()] = target;

                // Find the channel index for this target note to get its volume param
                const char* paramPrefix = nullptr;
                for (auto& d : kDrums)
                    if (d.midiNote == target) { paramPrefix = d.prefix; break; }

                float gain = paramPrefix
                    ? apvts.getRawParameterValue(juce::String(paramPrefix) + "_vol")->load()
                    : 1.0f;
                int scaledVel = juce::jlimit(1, 127, (int)(msg.getVelocity() * gain));

                remapped.addEvent(
                    juce::MidiMessage::noteOn(msg.getChannel(), target, (juce::uint8)scaledVel),
                    meta.samplePosition);
            }
        }
        else if (msg.isNoteOff())
        {
            int rbNote = msg.getNoteNumber();
            if (rbNote >= 96 && rbNote <= 112)
            {
                int t = activeTarget[rbNote];
                if (t >= 0)
                {
                    remapped.addEvent(
                        juce::MidiMessage::noteOff(msg.getChannel(), t),
                        meta.samplePosition);
                    activeTarget[rbNote] = -1;
                }
            }
            else
            {
                remapped.addEvent(msg, meta.samplePosition);
            }
        }
        else
        {
            remapped.addEvent(msg, meta.samplePosition);
        }
    }

    sampler.renderNextBlock(buffer, remapped, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* RBDrumSamplerAudioProcessor::createEditor()
{
    return new RBDrumSamplerAudioProcessorEditor(*this);
}

void RBDrumSamplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.state.createXml())
        copyXmlToBinary(*xml, destData);
}

void RBDrumSamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

void RBDrumSamplerAudioProcessor::loadSamples()
{
    sampler.clearSounds();
    sounds.fill(nullptr);

    struct SampleEntry {
        const void* data;
        int         size;
        int         channelIndex;
    };

    static const SampleEntry entries[] = {
        { BinaryData::Kick1_ogg,     BinaryData::Kick1_oggSize,     0 },
        { BinaryData::Snare1_ogg,    BinaryData::Snare1_oggSize,    1 },
        { BinaryData::HhClosed_ogg,  BinaryData::HhClosed_oggSize,  2 },
        { BinaryData::Ride1_ogg,     BinaryData::Ride1_oggSize,     3 },
        { BinaryData::Crsh1_ogg,     BinaryData::Crsh1_oggSize,     4 },
        { BinaryData::Crsh2_ogg,     BinaryData::Crsh2_oggSize,     5 },
        { BinaryData::HiMidTom_ogg,  BinaryData::HiMidTom_oggSize,  6 },
        { BinaryData::LoMidTom_ogg,  BinaryData::LoMidTom_oggSize,  7 },
        { BinaryData::LowFlrTom_ogg, BinaryData::LowFlrTom_oggSize, 8 },
    };

    for (auto& e : entries)
    {
        auto& d = kDrums[e.channelIndex];
        auto stream = std::make_unique<juce::MemoryInputStream>(e.data, (size_t)e.size, false);
        auto reader = std::unique_ptr<juce::AudioFormatReader>(
            formatManager.createReaderFor(std::move(stream)));
        if (!reader) continue;

        juce::BigInteger noteRange;
        noteRange.setBit(d.midiNote);
        auto* sound = new juce::SamplerSound(
            d.label, *reader, noteRange, d.midiNote,
            0.001f,  // attack
            0.1f,    // release
            10.0f    // max sample length in seconds
        );
        sampler.addSound(sound);
        sounds[e.channelIndex] = sound;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RBDrumSamplerAudioProcessor();
}
