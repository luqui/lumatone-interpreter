#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

private:
    static BusesProperties getBusesProperties();

    std::pair<int, float> lumaNoteToMidiNote (int ch, int note) const;
    std::pair<int, int> lumaNoteToLocalCoord (int note) const;

    juce::MPEChannelAssigner m_assigner;

    friend class LumatoneInterpreterEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumatoneInterpreterProcessor)
};
