if (idx < upper.length() && juce::CharacterFunctions::isDigit (upper[idx]))
    return juce::jlimit (0, 127, 12 * (upper[idx] - '0' + 1) + semi);
