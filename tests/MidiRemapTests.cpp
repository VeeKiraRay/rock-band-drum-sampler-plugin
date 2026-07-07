#include <JuceHeader.h>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <tuple>
#include <vector>

#include "../src/PluginProcessor.h"

namespace
{
    juce::MidiBuffer noteOnBuffer(std::initializer_list<std::tuple<int, int, int>> notes)
    {
        juce::MidiBuffer buf;
        for (auto& [note, vel, pos] : notes)
            buf.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)vel), pos);
        return buf;
    }

    struct Event
    {
        bool isOn;
        int  note;
        int  velocity;
        int  samplePos;
    };

    std::vector<Event> collectNotes(const juce::MidiBuffer& buf)
    {
        std::vector<Event> out;
        for (auto meta : buf)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
                out.push_back({ true, msg.getNoteNumber(), (int)msg.getVelocity(), meta.samplePosition });
            else if (msg.isNoteOff())
                out.push_back({ false, msg.getNoteNumber(), (int)msg.getVelocity(), meta.samplePosition });
        }
        return out;
    }

    bool hasNoteOn(const std::vector<Event>& events, int note)
    {
        return std::any_of(events.begin(), events.end(),
            [note](const Event& e) { return e.isOn && e.note == note; });
    }
}

TEST_CASE("Kick passthrough", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 96, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].isOn);
    CHECK(events[0].note == 96);
    CHECK(events[0].velocity == 100);
}

TEST_CASE("Kick 2 passthrough", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 95, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 95);
}

TEST_CASE("Snare passthrough", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 97, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 97);
}

TEST_CASE("Hi-hat normal", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 98);
}

TEST_CASE("Yellow tom override", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 }, { 110, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 110);
}

TEST_CASE("Blue tom override", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 99, 100, 0 }, { 111, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 111);
}

TEST_CASE("Green tom override", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 100, 100, 0 }, { 112, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].note == 112);
}

TEST_CASE("Y+G combo", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 }, { 100, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 2);
    CHECK(hasNoteOn(events, 101));
    CHECK(hasNoteOn(events, 100));
}

TEST_CASE("Combo suppressed by marker", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 }, { 100, 100, 0 }, { 110, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 2);
    CHECK(hasNoteOn(events, 110));
    CHECK(hasNoteOn(events, 100));
    CHECK_FALSE(hasNoteOn(events, 101));
}

TEST_CASE("Markers alone are silent", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 110, 100, 0 }, { 111, 100, 0 }, { 112, 100, 0 } }), out);

    CHECK(out.getNumEvents() == 0);
}

TEST_CASE("Other difficulty lanes dropped", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 60, 100, 0 }, { 72, 100, 0 }, { 84, 100, 0 }, { 88, 100, 0 } }), out);

    CHECK(out.getNumEvents() == 0);
}

TEST_CASE("Note-off follows remap across blocks", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;

    juce::MidiBuffer out1;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 }, { 110, 100, 0 } }), out1);

    juce::MidiBuffer in2;
    in2.addEvent(juce::MidiMessage::noteOff(1, 98), 0);
    juce::MidiBuffer out2;
    proc.remapMidi(in2, out2);

    auto events2 = collectNotes(out2);
    REQUIRE(events2.size() == 1);
    CHECK_FALSE(events2[0].isOn);
    CHECK(events2[0].note == 110);
}

TEST_CASE("Orphan note-off dropped", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;

    juce::MidiBuffer in;
    in.addEvent(juce::MidiMessage::noteOff(1, 97), 0);
    juce::MidiBuffer out;
    proc.remapMidi(in, out);

    CHECK(out.getNumEvents() == 0);
}

TEST_CASE("Non-note messages pass through unchanged", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;

    juce::MidiBuffer in;
    in.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 10);
    in.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 20);
    juce::MidiBuffer out;
    proc.remapMidi(in, out);

    REQUIRE(out.getNumEvents() == 2);
    std::vector<juce::MidiMessage> outMsgs;
    for (auto meta : out)
        outMsgs.push_back(meta.getMessage());

    CHECK(outMsgs[0].isController());
    CHECK(outMsgs[0].getControllerNumber() == 64);
    CHECK(outMsgs[1].isPitchWheel());
}

TEST_CASE("Velocity scaling clamps at 127 for high gain", "[remap]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.apvts.getParameter("kick_vol")->setValueNotifyingHost(1.0f);  // normalized 1.0 -> raw 2.0

    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 96, 100, 0 } }), out);

    auto events = collectNotes(out);
    REQUIRE(events.size() == 1);
    CHECK(events[0].velocity == 127);
}

TEST_CASE("Vol = 0.0 should be silent", "[remap][!mayfail]")
{
    // Documents CODE_REVIEW.md #1.1: velocity-as-gain clamps to a floor of 1, so
    // vol = 0.0 currently still triggers the sample at velocity 1 instead of being silent.
    RBDrumSamplerAudioProcessor proc;
    proc.apvts.getParameter("kick_vol")->setValueNotifyingHost(0.0f);

    juce::MidiBuffer out;
    proc.remapMidi(noteOnBuffer({ { 96, 127, 0 } }), out);

    CHECK(out.getNumEvents() == 0);
}

TEST_CASE("Y+G combo detected across block boundary", "[remap][!mayfail]")
{
    // Documents CODE_REVIEW.md #3.1: combo detection only looks within a single block, so
    // a marker/cymbal pair split across two processBlock calls currently plays hi-hat +
    // crash 1 instead of crash 2.
    RBDrumSamplerAudioProcessor proc;

    juce::MidiBuffer out1;
    proc.remapMidi(noteOnBuffer({ { 98, 100, 0 } }), out1);

    juce::MidiBuffer out2;
    proc.remapMidi(noteOnBuffer({ { 100, 100, 0 } }), out2);

    auto events2 = collectNotes(out2);
    CHECK(hasNoteOn(events2, 101));
}
