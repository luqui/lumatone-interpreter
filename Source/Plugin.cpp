#include "Plugin.h"

#include "Editor.h"

#include <juce_audio_basics/juce_audio_basics.h>

LumatoneInterpreterProcessor::LumatoneInterpreterProcessor() : AudioProcessor (getBusesProperties())
{
    // Initialize available tuning systems
    m_availableTunings.push_back (
        TuningSystem ("31 EDO", std::pow (2.0, 5.0 / 31.0), std::pow (2.0, 3.0 / 31.0), "31-tone equal temperament"));
    m_availableTunings.push_back (
        TuningSystem ("31-esque Regression", 1.118755, 1.068773, "Regression-based approximation of 31 EDO"));

    // Initialize the velocity fixup file path
    auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    auto lumatoneDir = appDataDir.getChildFile ("LumatoneInterpreter");
    if (! lumatoneDir.exists())
        lumatoneDir.createDirectory();

    m_velocityFixupFile = lumatoneDir.getChildFile ("velocity_fixups.xml");

    loadVelocityFixups();
}

bool LumatoneInterpreterProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void LumatoneInterpreterProcessor::prepareToPlay (double /*newSampleRate*/, int /*samplesPerBlock*/)
{
    reset();
}

void LumatoneInterpreterProcessor::releaseResources() {}

void LumatoneInterpreterProcessor::processBlock (juce::AudioBuffer<float>& audioIn, juce::MidiBuffer& midiMessages)
{
    audioIn.clear();

    juce::MidiBuffer midiOut;
    for (auto event : midiMessages) {
        juce::MidiMessage message = event.getMessage();

        if (message.getChannel() == 1) {
            // Pass through for things like pitch bend, program change
            midiOut.addEvent (message, event.samplePosition);
            continue;
        }

        int initialPressure = 0;
        if (message.isController()) {
            if (message.getControllerValue() == 0) {
                message = juce::MidiMessage::noteOff (message.getChannel(), message.getControllerNumber());
            }
            else {
                if (auto found = m_noteToChannel.find ({message.getChannel(), message.getControllerNumber()});
                    found != m_noteToChannel.end()) {
                    message = juce::MidiMessage::aftertouchChange (
                        message.getChannel(), message.getControllerNumber(), message.getControllerValue());
                }
                else {
                    // As an approximation of velocity, we treat the first nonzero controller value as the note-on
                    // velocity. Let's see if it works.
                    initialPressure = message.getControllerValue();
                    message = juce::MidiMessage::noteOn (
                        message.getChannel(),
                        message.getControllerNumber(),
                        (juce::uint8) message.getControllerValue());
                }
            }
        }

        if (message.isNoteOn()) {
            auto noteIn = message.getNoteNumber();
            auto channelIn = message.getChannel();
            auto velocity = (float) message.getVelocity();

            // Track the most recent key
            m_mostRecentKey = {channelIn, noteIn};

            auto [noteOut, bendOut] = lumaNoteToMidiNote (channelIn, noteIn);
            auto chOut = allocateChannel (channelIn, noteIn);
            velocity = velocityFixup (channelIn, noteIn, velocity);

            // Apply global velocity power curve
            velocity = std::pow (velocity / 127.0f, m_globalVelocityPower) * 127.0f;

            // Clamp and convert to int for MIDI output
            auto velocityOut = (juce::uint8) std::clamp ((int) std::round (velocity), 1, 127);

            midiOut.addEvent (
                juce::MidiMessage::pitchWheel (
                    chOut, std::clamp ((int) std::round (16383.0f * ((bendOut / 48.0f) / 2.0f + 0.5f)), 0, 16383)),
                event.samplePosition);
            midiOut.addEvent (juce::MidiMessage::channelPressureChange (chOut, initialPressure), event.samplePosition);
            midiOut.addEvent (juce::MidiMessage::noteOn (chOut, noteOut, velocityOut), event.samplePosition);
        }
        else if (message.isNoteOff()) {
            int noteIn = message.getNoteNumber();
            int channelIn = message.getChannel();

            auto [noteOut, bendOut] = lumaNoteToMidiNote (channelIn, noteIn);
            auto chOut = deallocateChannel (channelIn, noteIn);

            if (chOut != -1) {
                midiOut.addEvent (juce::MidiMessage::noteOff (chOut, noteOut), event.samplePosition);
            }
        }
        else if (message.isAftertouch()) {
            // To channel pressure
            int channelIn = message.getChannel();
            int noteIn = message.getNoteNumber();
            int pressure = message.getAfterTouchValue();

            if (auto found = m_noteToChannel.find ({channelIn, noteIn}); found != m_noteToChannel.end()) {
                auto chOut = found->second;
                midiOut.addEvent (juce::MidiMessage::channelPressureChange (chOut, pressure), event.samplePosition);
            }
        }
    }

    midiMessages.swapWith (midiOut);
}

int LumatoneInterpreterProcessor::allocateChannel (int ch, int note)
{
    auto noteId = m_nextNoteId++;
    // Use the same channel for exactly the same note (lumatone-wise)
    if (auto found = m_noteToChannel.find ({ch, note}); found != m_noteToChannel.end()) {
        return found->second;
    }

    // Look for the least recently used free channel
    int channel = -1;
    int lruId = INT_MAX;
    for (int i = 1; i < 16; ++i) {
        if (m_notesPerChannel[i] == 0) {
            if (m_channelLru[i] < lruId) {
                lruId = m_channelLru[i];
                channel = i;
            }
        }
    }
    if (channel != -1) {
        m_noteToChannel[{ch, note}] = channel;
        m_notesPerChannel[channel]++;
        m_channelLru[channel] = noteId;
        return channel;
    }

    // Otherwise, use the least recently used channel with the fewest notes
    int minNotes = INT_MAX;
    for (int i = 1; i < 16; ++i) {
        if (m_notesPerChannel[i] < minNotes || (m_notesPerChannel[i] == minNotes && m_channelLru[i] < lruId)) {
            minNotes = m_notesPerChannel[i];
            lruId = m_channelLru[i];
            channel = i;
        }
    }

    jassert (channel != -1);
    m_noteToChannel[{ch, note}] = channel;
    m_notesPerChannel[channel]++;
    m_channelLru[channel] = noteId;
    return channel;
}

int LumatoneInterpreterProcessor::deallocateChannel (int ch, int note)
{
    if (auto found = m_noteToChannel.find ({ch, note}); found != m_noteToChannel.end()) {
        auto channel = found->second;
        m_notesPerChannel[channel]--;
        m_noteToChannel.erase (found);
        return channel;
    }
    return -1;
}

float LumatoneInterpreterProcessor::velocityFixup (int ch, int note, int vel) const
{
    // Check for user-defined fixups first
    auto key = std::make_pair (ch, note);
    if (auto found = m_velocityFixups.find (key); found != m_velocityFixups.end()) {
        float pow = found->second;
        float out = std::pow (vel / 127.0f, pow) * 127.0f;
        return out;
    }

    return (float) vel;
}

std::pair<int, float> LumatoneInterpreterProcessor::lumaNoteToMidiNote (int ch, int note) const
{
    int x, y;
    std::tie (x, y) = lumaNoteToLocalCoord (note);

    x += 5 * (ch - 2);
    y += 2 * (ch - 2);

    // And re-center so the middle is (10,9)
    x -= 10;
    y -= 9;

    // Use the selected tuning system
    const auto& tuning = getCurrentTuning();
    double a = tuning.a;
    double b = tuning.b;

    double hz = 261.62 * std::pow (a, x) * std::pow (b, y);

    double midiNote = 12.0 * std::log2 (hz / 440.0) + 69.0;
    int midiNoteOut = std::clamp ((int) std::round (midiNote), 0, 127);
    double bendOut = midiNote - midiNoteOut;
    return {midiNoteOut, (float) bendOut};
}

std::pair<int, int> LumatoneInterpreterProcessor::lumaNoteToLocalCoord (int note) const
{
    switch (note) {
    case 0:
        return {0, 0};
    case 1:
        return {1, 0};
    case 2:
        return {0, 1};
    case 3:
        return {1, 1};
    case 4:
        return {2, 1};
    case 5:
        return {3, 1};
    case 6:
        return {4, 1};
    case 7:
        return {-1, 2};
    case 8:
        return {0, 2};
    case 9:
        return {1, 2};
    case 10:
        return {2, 2};
    case 11:
        return {3, 2};
    case 12:
        return {4, 2};
    case 13:
        return {-1, 3};
    case 14:
        return {0, 3};
    case 15:
        return {1, 3};
    case 16:
        return {2, 3};
    case 17:
        return {3, 3};
    case 18:
        return {4, 3};
    case 19:
        return {-2, 4};
    case 20:
        return {-1, 4};
    case 21:
        return {0, 4};
    case 22:
        return {1, 4};
    case 23:
        return {2, 4};
    case 24:
        return {3, 4};
    case 25:
        return {-2, 5};
    case 26:
        return {-1, 5};
    case 27:
        return {0, 5};
    case 28:
        return {1, 5};
    case 29:
        return {2, 5};
    case 30:
        return {3, 5};
    case 31:
        return {-3, 6};
    case 32:
        return {-2, 6};
    case 33:
        return {-1, 6};
    case 34:
        return {0, 6};
    case 35:
        return {1, 6};
    case 36:
        return {2, 6};
    case 37:
        return {-3, 7};
    case 38:
        return {-2, 7};
    case 39:
        return {-1, 7};
    case 40:
        return {0, 7};
    case 41:
        return {1, 7};
    case 42:
        return {2, 7};
    case 43:
        return {-4, 8};
    case 44:
        return {-3, 8};
    case 45:
        return {-2, 8};
    case 46:
        return {-1, 8};
    case 47:
        return {0, 8};
    case 48:
        return {1, 8};
    case 49:
        return {-3, 9};
    case 50:
        return {-2, 9};
    case 51:
        return {-1, 9};
    case 52:
        return {0, 9};
    case 53:
        return {1, 9};
    case 54:
        return {-1, 10};
    case 55:
        return {0, 10};
    }
    return {0, 0};
}

bool LumatoneInterpreterProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LumatoneInterpreterProcessor::createEditor()
{
    return new LumatoneInterpreterEditor (*this);
}

const juce::String LumatoneInterpreterProcessor::getName() const
{
    return "LumatoneInterpreter";
}
bool LumatoneInterpreterProcessor::acceptsMidi() const
{
    return true;
}
bool LumatoneInterpreterProcessor::producesMidi() const
{
    return true;
}
double LumatoneInterpreterProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LumatoneInterpreterProcessor::getNumPrograms()
{
    return 0;
}

int LumatoneInterpreterProcessor::getCurrentProgram()
{
    return 0;
}

void LumatoneInterpreterProcessor::setCurrentProgram (int) {}

const juce::String LumatoneInterpreterProcessor::getProgramName (int)
{
    return "None";
}

void LumatoneInterpreterProcessor::changeProgramName (int, const juce::String&) {}

void LumatoneInterpreterProcessor::getStateInformation (juce::MemoryBlock&) {}

void LumatoneInterpreterProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor::BusesProperties LumatoneInterpreterProcessor::getBusesProperties()
{
    return BusesProperties().withOutput ("No Output", juce::AudioChannelSet::stereo(), true);
}

juce::AudioProcessor* createPluginFilter()
{
    return std::make_unique<LumatoneInterpreterProcessor>().release();
}

float LumatoneInterpreterProcessor::getVelocityFixup (int ch, int note) const
{
    auto key = std::make_pair (ch, note);
    if (auto found = m_velocityFixups.find (key); found != m_velocityFixups.end()) {
        return found->second;
    }
    return 1.0f; // Default value
}

void LumatoneInterpreterProcessor::setVelocityFixup (int ch, int note, float powerValue)
{
    auto key = std::make_pair (ch, note);
    if (powerValue == 1.0f) {
        // Remove the fixup if it's the default value
        m_velocityFixups.erase (key);
    }
    else {
        m_velocityFixups[key] = powerValue;
    }
    saveVelocityFixups();
}

void LumatoneInterpreterProcessor::setGlobalVelocityPower (float power)
{
    m_globalVelocityPower = power;
    saveVelocityFixups();
}

void LumatoneInterpreterProcessor::setCurrentTuningIndex (int index)
{
    if (index >= 0 && index < static_cast<int> (m_availableTunings.size())) {
        m_currentTuningIndex = index;
        saveVelocityFixups(); // We'll save tuning state along with other settings
    }
}

void LumatoneInterpreterProcessor::saveVelocityFixups()
{
    juce::XmlElement root ("VelocityFixups");

    // Save global velocity power setting
    root.setAttribute ("globalVelocityPower", (double) m_globalVelocityPower);

    // Save current tuning index
    root.setAttribute ("currentTuningIndex", m_currentTuningIndex);

    for (const auto& [key, value] : m_velocityFixups) {
        auto* fixupElement = root.createNewChildElement ("Fixup");
        fixupElement->setAttribute ("channel", key.first);
        fixupElement->setAttribute ("note", key.second);
        fixupElement->setAttribute ("power", (double) value);
    }

    if (! root.writeTo (m_velocityFixupFile)) {
        std::cout << "Failed to save velocity fixups to " << m_velocityFixupFile.getFullPathName() << std::endl;
    }
}

void LumatoneInterpreterProcessor::loadVelocityFixups()
{
    if (! m_velocityFixupFile.exists())
        return;

    auto xml = juce::XmlDocument::parse (m_velocityFixupFile);
    if (xml == nullptr) {
        std::cout << "Failed to parse velocity fixups file" << std::endl;
        return;
    }

    m_velocityFixups.clear();

    // Load global velocity power setting
    m_globalVelocityPower = (float) xml->getDoubleAttribute ("globalVelocityPower", 1.0);

    // Load current tuning index
    m_currentTuningIndex = xml->getIntAttribute ("currentTuningIndex", 0);
    // Ensure the loaded index is valid
    if (m_currentTuningIndex < 0 || m_currentTuningIndex >= static_cast<int> (m_availableTunings.size())) {
        m_currentTuningIndex = 0;
    }

    for (auto* fixupElement : xml->getChildIterator()) {
        if (fixupElement->hasTagName ("Fixup")) {
            int channel = fixupElement->getIntAttribute ("channel");
            int note = fixupElement->getIntAttribute ("note");
            float power = (float) fixupElement->getDoubleAttribute ("power");

            m_velocityFixups[{channel, note}] = power;
        }
    }
}
