#pragma once
#include "PluginProcessor.h"
#include "ThemeSettings.h"

struct LogisthColours {
    juce::Colour body{0xffe8e9e2}, panel{0xfff4f3ed}, line{0xffced2c9}, ink{0xff283632},
        muted{0xff667268}, green{0xff406f5d}, mint{0xffbdd8bb}, screen{0xff243430},
        screenLine{0xff41514b}, screenText{0xffe6ece1};
    juce::Colour knobColour{0xff62acff}, knobTrack{0xff283a50};
    juce::String fontFamily = "Open Sans";
    int backgroundStyle = 0, knobStyle = 0;
    juce::Font font(float size, bool bold = false) const;
    void drawCaption(juce::Graphics &, const juce::String &, juce::Rectangle<int>, float,
                     juce::Colour, juce::Justification align = juce::Justification::centredLeft,
                     bool bold = false) const;
    bool darkTheme = false;
    int themeId = 0;
    void choosePalette(int);
    void applyPalette(const logisth::ThemeDefinition &);
};

class LogisthLook : public juce::LookAndFeel_V4, public LogisthColours {
  public:
    LogisthLook();
    void setDarkTheme(bool);
    void setTheme(int);
    void setPalette(const logisth::ThemeDefinition &);
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

class LogisthEditor : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit LogisthEditor(LogisthProcessor &);
    ~LogisthEditor() override;
    void resized() override;
    juce::Image snapshotPage(int scenario);
    juce::Image snapshotTheme(int theme);
    juce::String validateLayout();
    void setDarkTheme(bool);
    void setTheme(int);
    int selectedTheme() const;
    bool isDarkTheme() const;
    void setUIScale(double percent);
    double uiScale() const {
        return scalePercent;
    }

  private:
    class Console;
    LogisthLook look;
    std::unique_ptr<Console> console;
    void timerCallback() override;
    double scalePercent = 100;
    bool rememberSize = false, applyingScale = false, sizeDirty = false;
};
