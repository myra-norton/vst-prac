//
// Created by Myra Norton on 6/12/25.
//
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class Tempo
{
    public:
        void reset() noexcept;
        void update(const juce::AudioPlayHead* playhead) noexcept;
        double getMillisecondsForNoteLength(int index) const noexcept;
        double getTempo() const noexcept
        {
            return bpm;
        }
    private:
        double bpm = 120.0;
};