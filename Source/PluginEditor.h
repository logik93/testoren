#pragma once
#include "PluginProcessor.h"
class LogisthLook : public juce::LookAndFeel_V4 {
  public:
    LogisthLook();
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider &) override;
};
class LogisthEditor : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit LogisthEditor(LogisthProcessor &);
    ~LogisthEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    juce::Image snapshotPage(int page);

  private:
    class FormPage;
    void timerCallback() override;
    LogisthProcessor &processor;
    LogisthLook look;
    juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};
    std::array<std::unique_ptr<juce::Viewport>, 4> views;
    std::array<std::unique_ptr<FormPage>, 4> pages;
    juce::ComboBox presets;
    juce::Label presetLabel;
    juce::MidiKeyboardComponent keyboard;
    juce::Slider pitchWheel, modWheel;
};
