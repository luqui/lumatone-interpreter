#include "Plugin.h"

#include <juce_audio_basics/juce_audio_basics.h>

LumatoneInterpreterProcessor::LumatoneInterpreterProcessor() : AudioProcessor (getBusesProperties()) {}

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
            auto velocity = message.getVelocity();

            auto [noteOut, bendOut] = lumaNoteToMidiNote (channelIn, noteIn);
            auto chOut = allocateChannel (channelIn, noteIn);
            velocity = velocityFixup (channelIn, noteIn, velocity);

            midiOut.addEvent (
                juce::MidiMessage::pitchWheel (
                    chOut, std::clamp ((int) std::round (16383.0f * ((bendOut / 48.0f) / 2.0f + 0.5f)), 0, 16383)),
                event.samplePosition);
            midiOut.addEvent (juce::MidiMessage::channelPressureChange (chOut, initialPressure), event.samplePosition);
            midiOut.addEvent (juce::MidiMessage::noteOn (chOut, noteOut, velocity), event.samplePosition);
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
    for (int i = 2; i < 16; ++i) {
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
    for (int i = 2; i < 16; ++i) {
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

int LumatoneInterpreterProcessor::velocityFixup (int ch, int note, int vel) const
{
    int x, y;
    std::tie (x, y) = lumaNoteToLocalCoord (note);

    auto powScale = [&] (float pow) {
        int out = (int) std::round (std::pow (vel / 127.0f, pow) * 127.0f);
        std::cout << "Fixing up velocity from " << vel << " to " << out << std::endl;
        return out;
    };

    if (ch == 3 && x == 1 && y == 7) {
        return powScale (1.849f);
    }
    else if (ch == 4 && x == -2 && y == 5) {
        return powScale (0.495f);
    }
    else if (ch == 4 && x == 3 && y == 5) {
        return powScale (0.784f);
    }
    else if (ch == 4 && x == 3 && y == 4) {
        return powScale (0.5185f);
    }
    else
        return vel;
}

std::pair<int, float> LumatoneInterpreterProcessor::lumaNoteToMidiNote (int ch, int note) const
{
    int x, y;
    std::tie (x, y) = lumaNoteToLocalCoord (note);
    std::cout << "Coord = ch " << ch << ": " << x << ", " << y << std::endl;

    x += 5 * (ch - 2);
    y += 2 * (ch - 2);

    // And re-center so the middle is (10,9)
    x -= 10;
    y -= 9;

    // This is the tuning computation
    // 31EDO
    // float a = std::pow (2.0f, 5.0f / 31.0f);
    // float b = std::pow (2.0f, 3.0f / 31.0f);

    // 31-esque regression
    float a = 1.11794216f;
    float b = 1.07109905f;

    float hz = 261.62f * std::pow (a, x) * std::pow (b, y);

    float midiNote = 12.0f * std::log2 (hz / 440.0f) + 69.0f;
    int midiNoteOut = std::clamp ((int) std::round (midiNote), 0, 127);
    float bendOut = midiNote - midiNoteOut;
    return {midiNoteOut, bendOut};
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
    return new juce::GenericAudioProcessorEditor (*this);
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
