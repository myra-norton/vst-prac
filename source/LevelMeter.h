//
// Created by Myra Norton on 6/16/25.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Measurement.h"

class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter(Measurement& measurementL,
            Measurement& measurementR);
    ~LevelMeter() override;

    void paint(juce::Graphics&) override;
    void resized() override;
private:
    static constexpr float maxdB = 6.0f;
    static constexpr float mindB = -60.0f;
    static constexpr float stepdB = 6.0f;
    float maxPos = 0.0f;
    float minPos = 0.0f;
    static constexpr float clampdB = -120.0f;
    static constexpr float clampLevel = 0.000001f; //-120 dB
    float dbLevelL;
    float dbLevelR;
    int positionForLevel(float dbLevel) const noexcept
    {
      return int(std::round(juce::jmap(dbLevel,
        maxdB, mindB,
        maxPos, minPos)));
    }
    void timerCallback() override;
    void drawLevel(juce::Graphics& g, float level, int x, int width);
    static constexpr int refreshRate = 60;
    float decay = 0.0f;
    float levelL = clampLevel;
    float levelR = clampLevel;
    void updateLevel(float newLevel, float& smoothedLevel, float& leveldB) const;
    Measurement& measurementL;
    Measurement& measurementR;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
