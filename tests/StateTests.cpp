#include <JuceHeader.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>

#include "../src/PluginProcessor.h"

namespace
{
    constexpr const char* kSuffixes[] = { "_vol", "_atk", "_rel" };
}

TEST_CASE("All 30 parameters round-trip through state save/restore", "[state]")
{
    RBDrumSamplerAudioProcessor proc;

    // Push every parameter to a distinct, deterministic, non-default normalized value, then
    // read back what it actually settled on (ranges with a coarse step, like "_rel", snap
    // the requested value to their grid, so the achieved value can differ slightly from what
    // was requested - compare against the achieved value, not the raw request).
    std::vector<std::pair<juce::String, float>> idsAndValues;
    int i = 0;
    for (auto& d : kDrums)
    {
        for (auto* suffix : kSuffixes)
        {
            auto id = juce::String(d.prefix) + suffix;
            auto* param = proc.apvts.getParameter(id);
            REQUIRE(param != nullptr);
            param->setValueNotifyingHost(0.1f + 0.02f * (float)i);
            idsAndValues.push_back({ id, param->getValue() });
            ++i;
        }
    }

    // AudioProcessorValueTreeState syncs parameter values into its ValueTree via a 10 Hz
    // timer, not synchronously on setValueNotifyingHost - wait for the timer to become due,
    // then force it to fire, so sync has actually happened before we snapshot state.
    juce::Thread::sleep(120);
    juce::Timer::callPendingTimersSynchronously();

    juce::MemoryBlock state;
    proc.getStateInformation(state);

    RBDrumSamplerAudioProcessor fresh;
    fresh.setStateInformation(state.getData(), (int)state.getSize());

    for (auto& [id, expected] : idsAndValues)
    {
        auto* freshParam = fresh.apvts.getParameter(id);
        REQUIRE(freshParam != nullptr);
        CHECK(freshParam->getValue() == Catch::Approx(expected).margin(0.0001f));
    }
}

TEST_CASE("Mute state is not persisted", "[state]")
{
    RBDrumSamplerAudioProcessor proc;
    proc.muted.store(true);

    juce::MemoryBlock state;
    proc.getStateInformation(state);

    RBDrumSamplerAudioProcessor fresh;
    fresh.setStateInformation(state.getData(), (int)state.getSize());

    CHECK_FALSE(fresh.muted.load());
}

TEST_CASE("Garbage input to setStateInformation does not crash", "[state]")
{
    RBDrumSamplerAudioProcessor proc;

    auto* kickVol = proc.apvts.getParameter("kick_vol");
    REQUIRE(kickVol != nullptr);
    float defaultValue = kickVol->getValue();

    std::vector<char> garbage = { 'n', 'o', 't', ' ', 'x', 'm', 'l', 0, 1, 2, 3, 4 };
    proc.setStateInformation(garbage.data(), (int)garbage.size());

    // No crash is the primary assertion; also confirm defaults survive.
    CHECK(kickVol->getValue() == defaultValue);
}
