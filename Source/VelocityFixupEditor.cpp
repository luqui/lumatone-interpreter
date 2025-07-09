#include "VelocityFixupEditor.h"

VelocityFixupEditor::VelocityFixupEditor (LumatoneInterpreterProcessor& processor) : m_processor (processor)
{
    // Title label
    m_titleLabel.setText ("Velocity Fixup Editor", juce::dontSendNotification);
    m_titleLabel.setFont (juce::FontOptions (16.0f).withStyle ("bold"));
    m_titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (m_titleLabel);

    // Key info label
    m_keyInfoLabel.setFont (juce::FontOptions (14.0f));
    m_keyInfoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (m_keyInfoLabel);

    // Power value label
    m_powerLabel.setText ("Power Law Value:", juce::dontSendNotification);
    m_powerLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (m_powerLabel);

    // Power slider
    m_powerSlider.setRange (0.1, 3.0, 0.001);
    m_powerSlider.setValue (1.0);
    m_powerSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    m_powerSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
    m_powerSlider.setDoubleClickReturnValue (true, 1.0);
    m_powerSlider.onValueChange = [this]() { onSliderValueChanged(); };
    addAndMakeVisible (m_powerSlider);

    // Reset button
    m_resetButton.setButtonText ("Reset to 1.0");
    m_resetButton.onClick = [this]() { onResetButtonClicked(); };
    addAndMakeVisible (m_resetButton);

    updateCurrentKey();

    setSize (300, 200);
}

void VelocityFixupEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (40, 40, 40));

    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
}

void VelocityFixupEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    m_titleLabel.setBounds (bounds.removeFromTop (30));
    bounds.removeFromTop (10);

    m_keyInfoLabel.setBounds (bounds.removeFromTop (25));
    bounds.removeFromTop (15);

    m_powerLabel.setBounds (bounds.removeFromTop (20));
    bounds.removeFromTop (5);

    m_powerSlider.setBounds (bounds.removeFromTop (30));
    bounds.removeFromTop (15);

    m_resetButton.setBounds (bounds.removeFromTop (30));
}

void VelocityFixupEditor::updateCurrentKey()
{
    m_currentKey = m_processor.getMostRecentKey();

    // Create a descriptive string for the key
    auto [ch, note] = m_currentKey;
    auto [x, y] = m_processor.lumaNoteToLocalCoord (note);

    juce::String keyInfo = juce::String::formatted ("Channel: %d, Note: %d\nCoordinate: (%d, %d)", ch, note, x, y);
    m_keyInfoLabel.setText (keyInfo, juce::dontSendNotification);

    // Update slider to show current fixup value
    float currentFixup = m_processor.getVelocityFixup (ch, note);
    m_powerSlider.setValue (currentFixup, juce::dontSendNotification);
}

void VelocityFixupEditor::onSliderValueChanged()
{
    auto [ch, note] = m_currentKey;
    float powerValue = (float) m_powerSlider.getValue();
    m_processor.setVelocityFixup (ch, note, powerValue);
}

void VelocityFixupEditor::onResetButtonClicked()
{
    m_powerSlider.setValue (1.0);
    auto [ch, note] = m_currentKey;
    m_processor.setVelocityFixup (ch, note, 1.0f);
}
