#pragma once
#include "PluginProcessor.h"

struct LogisthColours {
    juce::Colour body{0xffe8e9e2}, panel{0xfff4f3ed}, line{0xffced2c9}, ink{0xff283632},
        muted{0xff667268}, green{0xff406f5d}, mint{0xffbdd8bb}, screen{0xff243430},
        screenLine{0xff41514b}, screenText{0xffe6ece1};
    bool darkTheme = false;
    void choosePalette(bool);
};

class LogisthLook : public juce::LookAndFeel_V4, public LogisthColours {
  public:
    LogisthLook();
    void setDarkTheme(bool);
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override;
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider &) override;
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider &) override;
    juce::Slider::SliderLayout getSliderLayout(juce::Slider &) override;
    juce::Label *createSliderTextBox(juce::Slider &) override;
    void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int,
                      juce::ComboBox &) override;
    juce::Font getComboBoxFont(juce::ComboBox &) override;
    void positionComboBoxText(juce::ComboBox &, juce::Label &) override;
    juce::Font getPopupMenuFont() override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox &,
                                                            juce::Label &) override;
    void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
    void drawCornerResizer(juce::Graphics &, int, int, bool, bool) override;

  private:
    juce::Typeface::Ptr regular, semibold;
};

class LogisthEditor : public juce::AudioProcessorEditor {
  public:
    explicit LogisthEditor(LogisthProcessor &);
    ~LogisthEditor() override;
    void resized() override;
    juce::Image snapshotPage(int scenario);
    juce::String validateLayout();
    void setDarkTheme(bool);
    bool isDarkTheme() const;

  private:
    class Console;
    LogisthLook look;
    std::unique_ptr<Console> console;
};
