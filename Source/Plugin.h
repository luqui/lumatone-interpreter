#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// hash for std::pair
namespace std
{
template <typename T, typename U>
struct hash<std::pair<T, U>>
{
    size_t operator() (const std::pair<T, U>& p) const { return std::hash<T> {}(p.first) ^ std::hash<U> {}(p.second); }
};
} // namespace std

// Forward declaration for the velocity fixup editor
class VelocityFixupEditor;

/** As the name suggest, this class does the actual audio processing. */
class LumatoneInterpreterProcessor : public juce::AudioProcessor
{
public:
    LumatoneInterpreterProcessor();
    ~LumatoneInterpreterProcessor() override = default;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void prepareToPlay (double newSampleRate, int /*samplesPerBlock*/) override;
    void releaseResources() override;

    bool supportsDoublePrecisionProcessing() const override { return false; }

    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int getActiveVoices() const { return m_noteToChannel.size(); }

    // Velocity fixup functionality
    std::pair<int, int> getMostRecentKey() const { return m_mostRecentKey; }
    float getVelocityFixup (int ch, int note) const;
    void setVelocityFixup (int ch, int note, float powerValue);
    void saveVelocityFixups();
    void loadVelocityFixups();

    // Global velocity sensitivity
    float getGlobalVelocityPower() const { return m_globalVelocityPower; }
    void setGlobalVelocityPower (float power);

private:
    static BusesProperties getBusesProperties();

    std::pair<int, float> lumaNoteToMidiNote (int ch, int note) const;
    std::pair<int, int> lumaNoteToLocalCoord (int note) const;
    float velocityFixup (int ch, int note, int vel) const;

    int m_nextNoteId = 0;
    std::unordered_map<int, int> m_channelLru;
    std::unordered_map<std::pair<int, int>, int> m_noteToChannel;
    std::unordered_map<int, int> m_notesPerChannel;

    // Velocity fixup data
    std::unordered_map<std::pair<int, int>, float> m_velocityFixups;
    std::pair<int, int> m_mostRecentKey {0, 0};
    juce::File m_velocityFixupFile;
    float m_globalVelocityPower = 1.0f;

    int allocateChannel (int ch, int note);
    int deallocateChannel (int ch, int note);

    friend class LumatoneInterpreterEditor;
    friend class VelocityFixupEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumatoneInterpreterProcessor)
};
