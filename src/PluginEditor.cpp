#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int kLabelCol  = 55;   // left column width for row labels
    constexpr int kColWidth  = 73;   // per-drum column width  (55 + 9*73 = 712 → see kEditorW)
    constexpr int kSliderH   = 90;   // vertical slider track height
    constexpr int kTextBoxH  = 16;   // value text box below slider
    constexpr int kRowH      = kSliderH + kTextBoxH;  // 106
    constexpr int kRowGap    = 8;
    constexpr int kHeader    = 26;
    constexpr int kEditorW      = kLabelCol + kColWidth * 9;  // 712
    constexpr int kButtonRowH   = 52;   // 12px top pad + 32px button + 8px bottom pad
    constexpr int kEditorH      = kHeader + 3 * kRowH + 2 * kRowGap + kButtonRowH;  // 412

    constexpr const char* kVolTip = "Volume: scales this drum's level. "
                                    "1.0 = unity gain, 2.0 = double, 0.0 = silent.";
    constexpr const char* kAtkTip = "Attack: fade-in time at the start of each hit. "
                                    "0ms = instant (normal for drums). "
                                    "Higher values soften the initial transient.";
    constexpr const char* kRelTip = "Release: fade-out time after the hit ends. "
                                    "Low = tight/dry, cuts the sample short. "
                                    "High = lets the sample ring out naturally.";
}

static void setupSlider(juce::Slider& s, const char* tooltip)
{
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, kTextBoxH);
    s.setTooltip(tooltip);
}

static void setupRowLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(11.0f, juce::Font::bold));
    l.setJustificationType(juce::Justification::centredRight);
    l.setColour(juce::Label::textColourId, juce::Colour(0xffaaaacc));
}

RBDrumSamplerAudioProcessorEditor::RBDrumSamplerAudioProcessorEditor(
    RBDrumSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    auto& apvts = audioProcessor.apvts;

    setupRowLabel(volRowLabel, "VOLUME");
    setupRowLabel(atkRowLabel, "ATTACK");
    setupRowLabel(relRowLabel, "RELEASE");
    addAndMakeVisible(volRowLabel);
    addAndMakeVisible(atkRowLabel);
    addAndMakeVisible(relRowLabel);

    for (int i = 0; i < 9; ++i)
    {
        juce::String prefix = kDrums[i].prefix;

        drumLabels[i].setText(kDrums[i].label, juce::dontSendNotification);
        drumLabels[i].setFont(juce::Font(12.0f, juce::Font::bold));
        drumLabels[i].setJustificationType(juce::Justification::centred);
        drumLabels[i].setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(drumLabels[i]);

        setupSlider(volSliders[i], kVolTip);
        addAndMakeVisible(volSliders[i]);
        attachments[i * 3 + 0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, prefix + "_vol", volSliders[i]);

        setupSlider(atkSliders[i], kAtkTip);
        addAndMakeVisible(atkSliders[i]);
        attachments[i * 3 + 1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, prefix + "_atk", atkSliders[i]);

        setupSlider(relSliders[i], kRelTip);
        addAndMakeVisible(relSliders[i]);
        attachments[i * 3 + 2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, prefix + "_rel", relSliders[i]);
    }

    // Mute All button
    muteButton.setButtonText("Mute All");
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffcc2222));
    muteButton.setTooltip("Silence all drum output without changing any slider values. "
                          "Click again to unmute. State is not saved with the project.");
    muteButton.onClick = [this]
    {
        audioProcessor.muted.store(muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);

    // Reset to Default button
    resetButton.setButtonText("Reset to Default");
    resetButton.setTooltip("Reset all volume, attack, and release sliders to their default values.");
    resetButton.onClick = [this]
    {
        for (auto& d : kDrums)
        {
            juce::String p = d.prefix;
            for (auto* suffix : { "_vol", "_atk", "_rel" })
            {
                auto* param = audioProcessor.apvts.getParameter(p + suffix);
                if (param) param->setValueNotifyingHost(param->getDefaultValue());
            }
        }
    };
    addAndMakeVisible(resetButton);

    setSize(kEditorW, kEditorH);
}

RBDrumSamplerAudioProcessorEditor::~RBDrumSamplerAudioProcessorEditor() {}

void RBDrumSamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colour(0xff2a2a4e));
    for (int i = 0; i < 9; ++i)
        g.drawVerticalLine(kLabelCol + i * kColWidth, 0.0f, (float)getHeight());
}

void RBDrumSamplerAudioProcessorEditor::resized()
{
    // Header row: drum name labels
    for (int i = 0; i < 9; ++i)
        drumLabels[i].setBounds(kLabelCol + i * kColWidth, 0, kColWidth, kHeader);

    // Row 0: Volume
    int rowY = kHeader;
    volRowLabel.setBounds(0, rowY, kLabelCol - 4, kRowH);
    for (int i = 0; i < 9; ++i)
        volSliders[i].setBounds(kLabelCol + i * kColWidth, rowY, kColWidth, kRowH);

    // Row 1: Attack
    rowY += kRowH + kRowGap;
    atkRowLabel.setBounds(0, rowY, kLabelCol - 4, kRowH);
    for (int i = 0; i < 9; ++i)
        atkSliders[i].setBounds(kLabelCol + i * kColWidth, rowY, kColWidth, kRowH);

    // Row 2: Release
    rowY += kRowH + kRowGap;
    relRowLabel.setBounds(0, rowY, kLabelCol - 4, kRowH);
    for (int i = 0; i < 9; ++i)
        relSliders[i].setBounds(kLabelCol + i * kColWidth, rowY, kColWidth, kRowH);

    // Button row
    constexpr int kBtnW1 = 120, kBtnW2 = 160, kBtnGap = 16, kBtnH = 32;
    constexpr int kBtnTotalW = kBtnW1 + kBtnGap + kBtnW2;
    const int btnY = rowY + kRowH + 12;
    const int btnX = (kEditorW - kBtnTotalW) / 2;
    muteButton.setBounds (btnX,              btnY, kBtnW1, kBtnH);
    resetButton.setBounds(btnX + kBtnW1 + kBtnGap, btnY, kBtnW2, kBtnH);
}
