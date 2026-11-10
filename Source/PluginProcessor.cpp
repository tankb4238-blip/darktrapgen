juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkTrapAudioProcessor();
}
