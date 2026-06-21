#pragma once
#include <JuceHeader.h>

struct DrumChannel
{
    const char* prefix;
    const char* label;
    int         midiNote;
    float       defaultRelease;
};

static constexpr std::array<DrumChannel, 10> kDrums = {{
    { "kick",     "Kick (O)",      96,  0.40f },
    { "kick2",    "Kick 2 (O2)",   95,  0.40f },
    { "snare",    "Snare (R)",     97,  0.30f },
    { "hihat",    "Hi-Hat (Y)",    98,  0.30f },
    { "ride",     "Ride (B)",      99,  1.00f },
    { "crash1",   "Crash 1 (G)",  100,  1.50f },
    { "crash2",   "Crash 2 (YG)", 101,  1.50f },
    { "hitom",    "Hi Tom (Y*)",  110,  0.40f },
    { "midtom",   "Mid Tom (B*)", 111,  0.50f },
    { "floortom", "Flr Tom (G*)", 112,  0.60f },
}};

class RBDrumSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    RBDrumSamplerAudioProcessor();
    ~RBDrumSamplerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return true;  }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram(int) override                        {}
    const juce::String getProgramName(int) override             { return {}; }
    void changeProgramName(int, const juce::String&) override   {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<bool> muted { false };

private:
    juce::AudioFormatManager formatManager;
    juce::Synthesiser         sampler;
    std::array<juce::SamplerSound*, 10> sounds {};
    std::array<int, 128> activeTarget {};  // RB note → internal target (-1 = none active)

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadSamples();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RBDrumSamplerAudioProcessor)
};
