#pragma once

#include "Plugin.h"
#include "VelocityFixupEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class VelocityFixupWindow : public juce::DocumentWindow
{
public:
    VelocityFixupWindow (const juce::String& name, juce::Colour backgroundColour, int requiredButtons)
    : DocumentWindow (name, backgroundColour, requiredButtons)
    {}

    void closeButtonPressed() override
    {
        if (onWindowClosed)
            onWindowClosed();
    }

    std::function<void()> onWindowClosed;
};

class LumatoneInterpreterEditor
: public juce::AudioProcessorEditor
, private juce::Timer
{
public:
    LumatoneInterpreterEditor (LumatoneInterpreterProcessor& proc) : AudioProcessorEditor (proc)
    {
        addAndMakeVisible (m_activeVoicesLabel);

        m_velocityFixupButton.setButtonText ("Edit Velocity Fixup");
        m_velocityFixupButton.onClick = [this]() { openVelocityFixupEditor(); };
        addAndMakeVisible (m_velocityFixupButton);

        // Global velocity power slider
        m_globalVelocityPowerSlider.setRange (0.1, 10.0, 0.01);
        m_globalVelocityPowerSlider.setValue (
            static_cast<LumatoneInterpreterProcessor&> (processor).getGlobalVelocityPower(),
            juce::dontSendNotification);
        m_globalVelocityPowerSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
        m_globalVelocityPowerSlider.onValueChange = [this]() {
            auto& proc = static_cast<LumatoneInterpreterProcessor&> (processor);
            proc.setGlobalVelocityPower ((float) m_globalVelocityPowerSlider.getValue());
        };
        addAndMakeVisible (m_globalVelocityPowerSlider);

        m_globalVelocityPowerLabel.setText ("Global Velocity Power:", juce::dontSendNotification);
        m_globalVelocityPowerLabel.attachToComponent (&m_globalVelocityPowerSlider, true);

        startTimerHz (10);

        setSize ((int) (1.618f * 400), 400);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        m_velocityFixupButton.setBounds (bounds.removeFromTop (40));
        bounds.removeFromTop (8);
        m_globalVelocityPowerSlider.setBounds (bounds.removeFromTop (30));
        bounds.removeFromTop (8);
        m_activeVoicesLabel.setBounds (bounds);
    }

private:
    void timerCallback() override
    {
        auto& proc = static_cast<LumatoneInterpreterProcessor&> (processor);
        m_activeVoicesLabel.setText (
            "Active voices: " + juce::String (proc.getActiveVoices()), juce::dontSendNotification);

        // Update global velocity power slider to reflect current value
        m_globalVelocityPowerSlider.setValue (proc.getGlobalVelocityPower(), juce::dontSendNotification);
    }

    void openVelocityFixupEditor()
    {
        if (m_velocityFixupWindow == nullptr) {
            auto& proc = static_cast<LumatoneInterpreterProcessor&> (processor);
            auto editor = std::make_unique<VelocityFixupEditor> (proc);
            editor->updateCurrentKey();

            m_velocityFixupWindow = std::make_unique<VelocityFixupWindow> (
                "Velocity Fixup Editor", juce::Colours::darkgrey, juce::DocumentWindow::closeButton);

            m_velocityFixupWindow->setContentOwned (editor.release(), true);
            m_velocityFixupWindow->setResizable (false, false);
            m_velocityFixupWindow->centreWithSize (300, 200);
            m_velocityFixupWindow->setVisible (true);
            m_velocityFixupWindow->setAlwaysOnTop (true);

            // Set up close callback
            m_velocityFixupWindow->onWindowClosed = [this]() { m_velocityFixupWindow.reset(); };
        }
        else {
            // Update the existing window
            if (auto* editor = dynamic_cast<VelocityFixupEditor*> (m_velocityFixupWindow->getContentComponent())) {
                editor->updateCurrentKey();
            }
            m_velocityFixupWindow->toFront (true);
        }
    }

    juce::Label m_activeVoicesLabel;
    juce::TextButton m_velocityFixupButton;
    juce::Slider m_globalVelocityPowerSlider;
    juce::Label m_globalVelocityPowerLabel;
    std::unique_ptr<VelocityFixupWindow> m_velocityFixupWindow;
};