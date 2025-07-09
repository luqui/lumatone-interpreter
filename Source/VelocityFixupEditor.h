#pragma once

#include "Plugin.h"

#include <juce_gui_basics/juce_gui_basics.h>

class VelocityFixupEditor : public juce::Component
{
public:
    VelocityFixupEditor (LumatoneInterpreterProcessor& processor);
    ~VelocityFixupEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void updateCurrentKey();

private:
    void onSliderValueChanged();
    void onResetButtonClicked();

    LumatoneInterpreterProcessor& m_processor;

    juce::Label m_titleLabel;
    juce::Label m_keyInfoLabel;
    juce::Label m_powerLabel;
    juce::Slider m_powerSlider;
    juce::TextButton m_resetButton;

    std::pair<int, int> m_currentKey {0, 0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityFixupEditor)
};
