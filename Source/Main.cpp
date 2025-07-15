#include "Plugin.h"

// Include order matters here.
// clang-format off
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
// clang-format on

using namespace juce;

class LumatoneInterpreterApplication : public JUCEApplication
{
public:
    LumatoneInterpreterApplication() {}

    void initialise (const String& commandLineParameters) override
    {
        filterWindow =
            std::make_unique<StandaloneFilterWindow> (getApplicationName(), juce::Colours::black, nullptr, true);
        filterWindow->setTitleBarButtonsRequired (DocumentWindow::allButtons, false);

        // Set default device settings
        setDefaultDeviceSettings();

        filterWindow->setVisible (true);
        filterWindow->setResizable (true, true);
    }

    void shutdown() override {}

    const String getApplicationName() override { return juce::String ("Lumatone Interpreter"); }
    const String getApplicationVersion() override { return String ("0.01"); }
    bool moreThanOneInstanceAllowed() override { return true; }

private:
    void setDefaultDeviceSettings()
    {
        if (auto* pluginHolder = filterWindow->getPluginHolder()) {
            auto& deviceManager = pluginHolder->deviceManager;

            // Set default buffer size to 64 samples
            auto currentSetup = deviceManager.getAudioDeviceSetup();
            currentSetup.bufferSize = 64;

            // Try to set "Lumatone" as MIDI input device if available
            auto midiInputs = MidiInput::getAvailableDevices();
            for (const auto& input : midiInputs) {
                if (input.name.containsIgnoreCase ("Lumatone")) {
                    deviceManager.setMidiInputDeviceEnabled (input.identifier, true);
                    break;
                }
            }

            // Try to set "IAC Bus 1" as default MIDI output device if available
            auto midiOutputs = MidiOutput::getAvailableDevices();
            for (const auto& output : midiOutputs) {
                if (output.name.containsIgnoreCase ("IAC Driver Bus 1")) {
                    deviceManager.setDefaultMidiOutputDevice (output.identifier);
                    break;
                }
            }

            // Apply the audio device settings
            deviceManager.setAudioDeviceSetup (currentSetup, true);
        }
    }

    std::unique_ptr<juce::StandaloneFilterWindow> filterWindow;
};

START_JUCE_APPLICATION (LumatoneInterpreterApplication)
