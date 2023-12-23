/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SynthUsingMidiInputTutorial
 version:          2.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Synthesiser with midi input.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
struct SineWaveSound   : public juce::SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote    (int) override        { return true; }
    bool appliesToChannel (int) override        { return true; }
};

//==============================================================================
struct SineWaveVoice   : public juce::SynthesiserVoice
{
    SineWaveVoice(const juce::AudioSampleBuffer& wavetableToUse) : wavetable(wavetableToUse) {
        jassert(wavetable.getNumChannels() == 1);
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        FREQ = cyclesPerSecond;
        BASE_FREQ = cyclesPerSecond;
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        tableSizeOverSampleRate = (float)wavetable.getNumSamples() / getSampleRate();
        
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int newPitchWheelValue) override      {
        double wheelPos = ((float)newPitchWheelValue - 8192.0f) / 8192.0f; // ピッチホイールの値を-1.0から1.0の範囲に正規化
        double semitones = wheelPos * 2.0; // 範囲を2半音（全音）分にする
        pitchShift = std::pow(2.0, semitones / 12.0); // 倍音比に変換してピッチシフト量を計算
        FREQ = BASE_FREQ * pitchShift;
    }

    void aftertouchChanged(int newAftertouchValue) override
    {
        float aftertoutchPos = static_cast<float>(newAftertouchValue) / 127.0f; // アフタータッチの値を-1.0から1.0の範囲に正規化
        double semitones = aftertoutchPos * 2.0; // 範囲を2半音（全音）分にする
        pitchShift = std::pow(2.0, semitones / 12.0); // 倍音比に変換してピッチシフト量を計算
        FREQ = BASE_FREQ * pitchShift; //ピッチシフトを周波数に適用

    }

    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        tableDelta = FREQ * tableSizeOverSampleRate;
        /*
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {

                    auto currentSample = (float) (std::sin (currentAngle) * level * tailOff) ;

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta * pitchShift;
                    ++startSample;

                    tailOff *= 0.99; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    auto currentSample = (float) (std::sin (currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta * pitchShift;
                    ++startSample;
                }
            }
        }*/
        
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {
                    //auto index = (int)currentAngle % wavetable.getNumSamples();
                    auto tableSize = (unsigned int)wavetable.getNumSamples();

                    auto index0 = (unsigned int)currentIndex;              // [6]
                    auto index1 = index0 == (tableSize - 1) ? (unsigned int)0 : index0 + 1;
                    auto frac = currentIndex - (float)index0;              // [7]

                    auto* table = wavetable.getReadPointer(0);             // [8]
                    auto value0 = table[index0];
                    auto value1 = table[index1];

                    auto currentSample = (value0 + frac * (value1 - value0))*level*tailOff; // [9]

                    if ((currentIndex += tableDelta) > (float)tableSize)   // [10]
                        currentIndex -= (float)tableSize;
                    //auto currentSample = wavetable.getSample(0, index0) * level *tailOff;

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);

                    currentAngle += angleDelta * pitchShift;
                    ++startSample;
                    tailOff *= 0.99; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    //auto index = (int)currentAngle % wavetable.getNumSamples();
                    //auto currentSample = wavetable.getSample(0, index) * level;
                    auto tableSize = (unsigned int)wavetable.getNumSamples();

                    auto index0 = (unsigned int)currentIndex;              // [6]
                    auto index1 = index0 == (tableSize - 1) ? (unsigned int)0 : index0 + 1;
                    auto frac = currentIndex - (float)index0;              // [7]

                    auto* table = wavetable.getReadPointer(0);             // [8]
                    auto value0 = table[index0];
                    auto value1 = table[index1];

                    auto currentSample = (value0 + frac * (value1 - value0))*level; // [9]

                    if ((currentIndex += tableDelta) > (float)tableSize)   // [10]
                        currentIndex -= (float)tableSize;

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);

                    currentAngle += angleDelta * pitchShift;
                    ++startSample;
                }
            }
        }
      
    }

private:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
    double FREQ = 0;
    double BASE_FREQ = 0;
    double pitchShift = 1.0;
    double tableSizeOverSampleRate;
    const juce::AudioSampleBuffer& wavetable;
    float currentIndex = 0.0f,tableDelta=0.0f;
 
};

//============================================================================
class SynthAudioSource   : public juce::AudioSource
{
public:
    SynthAudioSource (juce::MidiKeyboardState& keyState)
        : keyboardState (keyState)
    {
        
        // wavetableの生成（wavetableSizeは適切なサイズに置き換える必要があります）
        const int wavetableSize = 1024; // 例として1024サイズのwavetableを作成
        juce::AudioSampleBuffer wavetableBuffer(1, wavetableSize);

        sineTable.setSize(1, (int)tableSize);
        sineTable.clear();
        auto* samples = sineTable.getWritePointer(0);                                   // [3]
        /*
        auto angleDelta = juce::MathConstants<double>::twoPi / (double)(tableSize - 1); // [4]
        auto currentAngle = 0.0;

        for (unsigned int i = 0; i < tableSize; ++i)
        {
            auto sample = std::sin(currentAngle);                                       // [5]
            samples[i] = (float)sample;
            currentAngle += angleDelta;
        }
        */
        int harmonics[] = { 1,3,5,7,9};
        float harmonicWeights[] = { 0.5f, 0.1f, 0.05f, 0.125f, 0.09f, 0.005f, 0.002f, 0.001f };

       jassert(juce::numElementsInArray(harmonics) == juce::numElementsInArray(harmonicWeights));

        //[3-2]波形を作成します。



        for (auto harmonic = 0; harmonic < juce::numElementsInArray(harmonics); ++harmonic)
        {
            auto angleDelta = juce::MathConstants<double>::twoPi / (double)(tableSize - 1) * harmonics[harmonic];
            auto currentAngle = 0.0;

            for (unsigned int i = 0; i < tableSize; ++i)
            {
                auto sample = std::sin(currentAngle);
                samples[i] += (float)sample * harmonicWeights[harmonic];
               // samples[i] += (float)sample ;
                currentAngle += angleDelta;
            }
        }

        for (auto i = 0; i < 4; ++i)
            synth.addVoice (new SineWaveVoice(sineTable));
        synth.addSound (new SineWaveSound());
        
    }

    void setUsingSineWaveSound()
    {
        synth.clearSounds();
    }

    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        synth.setCurrentPlaybackSampleRate (sampleRate);
        midiCollector.reset (sampleRate); // [10]
    }

    void releaseResources() override {}

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        juce::MidiBuffer incomingMidi;
        midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples); // [11]

        keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                             bufferToFill.numSamples, true);

        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                               bufferToFill.startSample, bufferToFill.numSamples);
    }

    juce::MidiMessageCollector* getMidiCollector()
    {
        return &midiCollector;
    }

private:
 
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;

    juce::AudioSampleBuffer sineTable;
    //const unsigned int tableSize = 1 << 7;
    const unsigned int tableSize = 1024;

};

//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent,
                               private juce::Timer
{
public:
    MainContentComponent()
        : synthAudioSource  (keyboardState),
          keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        addAndMakeVisible (midiInputListLabel);
        midiInputListLabel.setText ("MIDI Input:", juce::dontSendNotification);
        midiInputListLabel.attachToComponent (&midiInputList, true);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        addAndMakeVisible (midiInputList);
        midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");

        juce::StringArray midiInputNames;
        for (auto input : midiInputs)
            midiInputNames.add (input.name);

        midiInputList.addItemList (midiInputNames, 1);
        midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };

        for (auto input : midiInputs)
        {
            if (deviceManager.isMidiInputDeviceEnabled (input.identifier))
            {
                setMidiInput (midiInputs.indexOf (input));
                break;
            }
        }

        if (midiInputList.getSelectedId() == 0)
            setMidiInput (0);

        addAndMakeVisible (keyboardComponent);
        setAudioChannels (0, 2);
        //createWavetable();
        setSize (600, 190);
        startTimer (400);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void resized() override
    {
        midiInputList    .setBounds (200, 10, getWidth() - 210, 20);
        keyboardComponent.setBounds (10,  40, getWidth() - 20, getHeight() - 50);
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        synthAudioSource.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override
    {
        synthAudioSource.releaseResources();
    }

private:
    void timerCallback() override
    {
        keyboardComponent.grabKeyboardFocus();
        stopTimer();
    }

    void setMidiInput (int index)
    {
        auto list = juce::MidiInput::getAvailableDevices();

        deviceManager.removeMidiInputDeviceCallback (list[lastInputIndex].identifier,
                                                     synthAudioSource.getMidiCollector()); // [12]

        auto newInput = list[index];

        if (! deviceManager.isMidiInputDeviceEnabled (newInput.identifier))
            deviceManager.setMidiInputDeviceEnabled (newInput.identifier, true);

        deviceManager.addMidiInputDeviceCallback (newInput.identifier, synthAudioSource.getMidiCollector()); // [13]
        midiInputList.setSelectedId (index + 1, juce::dontSendNotification);

        lastInputIndex = index;
    }

    //==========================================================================
    juce::MidiKeyboardState keyboardState;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;


    juce::ComboBox midiInputList;
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
