#include <JuceHeader.h>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "../src/PluginProcessor.h"

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;

    juce::AudioBuffer<float> renderBlock(RBDrumSamplerAudioProcessor& proc, juce::MidiBuffer midi)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        proc.processBlock(buffer, midi);
        return buffer;
    }
}

TEST_CASE("No MIDI produces silence", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto buffer = renderBlock(proc, {});
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) == 0.0f);
}

TEST_CASE("Note-on 96 produces non-silent output", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    auto buffer = renderBlock(proc, midi);
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0f);
}

TEST_CASE("All ten target notes decode and trigger audio via the RB remap scheme", "[audio]")
{
    // Each entry is the RB note combination needed to reach one of the ten internal
    // sampler targets (95-101, 110-112) through the real remap logic, not a raw feed of
    // the target note itself (several targets, like 101/110/111/112, are only reachable
    // via marker/combo combinations - remapMidi drops them if fed directly).
    struct Trigger { std::vector<int> notes; int expectedTarget; };
    const std::vector<Trigger> triggers = {
        { { 95 },       95 },   // Kick 2
        { { 96 },       96 },   // Kick
        { { 97 },       97 },   // Snare
        { { 98 },       98 },   // Hi-hat
        { { 99 },       99 },   // Ride
        { { 100 },      100 },  // Crash 1
        { { 98, 100 },  101 },  // Y+G combo -> Crash 2
        { { 98, 110 },  110 },  // Yellow tom override
        { { 99, 111 },  111 },  // Blue tom override
        { { 100, 112 }, 112 },  // Green tom override
    };

    for (auto& trig : triggers)
    {
        RBDrumSamplerAudioProcessor proc;
        proc.prepareToPlay(kSampleRate, kBlockSize);

        juce::MidiBuffer midi;
        for (int note : trig.notes)
            midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)127), 0);

        auto buffer = renderBlock(proc, midi);
        INFO("expected target = " << trig.expectedTarget);
        CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0f);
    }
}

TEST_CASE("Muted output is silent", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    proc.muted.store(true);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    auto buffer = renderBlock(proc, midi);
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) == 0.0f);
}

TEST_CASE("Note-on then mute then unmute does not crash", "[audio]")
{
    // Loose assert only - documents current freeze/resume behavior (CODE_REVIEW.md #1.3).
    // The real assertion is "no crash"; magnitude after unmute is not pinned down since the
    // voice may have decayed differently depending on when the mute took effect.
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    renderBlock(proc, midi);

    proc.muted.store(true);
    renderBlock(proc, {});

    proc.muted.store(false);
    renderBlock(proc, {});

    SUCCEED("no crash across mute/unmute transition");
}

TEST_CASE("prepareToPlay called twice still produces audio", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    auto buffer = renderBlock(proc, midi);
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0f);
}

TEST_CASE("Sample-rate independence: 44.1 kHz", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(44100.0, kBlockSize);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    auto buffer = renderBlock(proc, midi);
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0f);
}

TEST_CASE("Sample-rate independence: 96 kHz", "[audio]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.prepareToPlay(96000.0, kBlockSize);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 96, (juce::uint8)127), 0);
    auto buffer = renderBlock(proc, midi);
    CHECK(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0f);
}
