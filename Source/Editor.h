#pragma once

#include "Plugin.h"

#include <juce_audio_processors/juce_audio_processors.h>

class LumatoneInterpreterEditor
: public juce::AudioProcessorEditor
, private juce::Timer
{
public:
    LumatoneInterpreterEditor (LumatoneInterpreterProcessor& proc) : AudioProcessorEditor (proc)
    {
        addAndMakeVisible (m_activeVoicesLabel);
        startTimerHz (10);

        setSize ((int) (1.618f * 400), 400);
    }

    void resized() override { m_activeVoicesLabel.setBounds (getLocalBounds().reduced (8)); }

private:
    void timerCallback() override
    {
        auto& proc = static_cast<LumatoneInterpreterProcessor&> (processor);
        m_activeVoicesLabel.setText (
            "Active voices: " + juce::String (proc.getActiveVoices()), juce::dontSendNotification);
    }

    juce::Label m_activeVoicesLabel;
};