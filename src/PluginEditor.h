#pragma once
#include "PluginProcessor.h"

class RBDrumSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RBDrumSamplerAudioProcessorEditor(RBDrumSamplerAudioProcessor&);
    ~RBDrumSamplerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    RBDrumSamplerAudioProcessor& audioProcessor;

    std::array<juce::Slider, 9> volSliders;
    std::array<juce::Slider, 9> atkSliders;
    std::array<juce::Slider, 9> relSliders;
    std::array<juce::Label,  9> drumLabels;

    juce::Label volRowLabel, atkRowLabel, relRowLabel;

    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 27> attachments;

    juce::TextButton muteButton, resetButton;

    juce::TooltipWindow tooltipWindow { this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RBDrumSamplerAudioProcessorEditor)
};
