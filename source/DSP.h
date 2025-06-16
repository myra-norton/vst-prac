//
// Created by Myra Norton on 6/12/25.
//
#pragma once
#include <cmath>

inline void panningEqualPower(float panning, float& left, float& right)
{
    float x = 0.7853981633974483f * (panning + 1.0f); //the coeff is pi/4
    left = std::cos(x);
    right = std::sin(x);
}