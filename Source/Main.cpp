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
        filterWindow->setVisible (true);
        filterWindow->setResizable (true, true);
    }

    void shutdown() override {}

    const String getApplicationName() override { return juce::String ("Lumatone Interpreter"); }
    const String getApplicationVersion() override { return String ("0.01"); }
    bool moreThanOneInstanceAllowed() override { return true; }

private:
    std::unique_ptr<juce::StandaloneFilterWindow> filterWindow;
};

START_JUCE_APPLICATION (LumatoneInterpreterApplication)
