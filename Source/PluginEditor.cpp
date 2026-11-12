#include "PluginEditor.h"
#include "BinaryData.h"
#include <set>
using namespace juce;
using namespace logisth;
namespace {
constexpr int canvasWidth = 1240, canvasHeight = 860;
Typeface::Ptr embeddedFace(bool bold) {
    static const auto regularFace = Typeface::createSystemTypefaceFor(
        BinaryData::OpenSansRegular_ttf, BinaryData::OpenSansRegular_ttfSize);
    static const auto semiboldFace = Typeface::createSystemTypefaceFor(
        BinaryData::OpenSansSemibold_ttf, BinaryData::OpenSansSemibold_ttfSize);
    return bold ? semiboldFace : regularFace;
}
Font font(float size, bool bold = false) {
    return Font(FontOptions(embeddedFace(bold)).withHeight(size));
}

bool dark(const Component &c) {
    for (auto *p = &c; p != nullptr; p = p->getParentComponent())
        if (p->getProperties().contains("dark"))
            return bool(p->getProperties()["dark"]);
    return false;
}
void drawCaption(Graphics &g, const String &value, Rectangle<int> bounds, float size, Colour colour,
                 Justification align = Justification::centredLeft, bool bold = false) {
    g.setFont(font(size, bold));
    g.setColour(colour);
    g.drawText(value, bounds, align, true);
}
void paintShell(Graphics &g, Rectangle<int> b, const String &title, const String &hint,
                const LogisthColours &colours) {
    g.setColour(Colours::black.withAlpha(colours.darkTheme ? .16f : .035f));
    g.fillRoundedRectangle(b.translated(0, 2).toFloat(), 8);
    g.setColour(colours.panel);
    g.fillRoundedRectangle(b.toFloat(), 8);
    g.setColour(colours.line);
    g.drawRoundedRectangle(b.toFloat().reduced(.5f), 8, 1);
    drawCaption(g, title, b.reduced(12, 0).withY(b.getY() + 8).withHeight(18), 11.5f, colours.ink,
                Justification::centredLeft, true);
    if (hint.isNotEmpty())
        drawCaption(g, hint, b.reduced(12, 0).withY(b.getY() + 8).withHeight(18), 9, colours.muted,
                    Justification::centredRight);
}
void capsule(Graphics &g, Rectangle<float> b, Colour fill, Colour outline) {
    g.setColour(fill);
    g.fillRoundedRectangle(b, 4);
    g.setColour(outline);
    g.drawRoundedRectangle(b.reduced(.5f), 4, 1);
}
} // namespace

void LogisthColours::choosePalette(bool enabled) {
    darkTheme = enabled;
    if (enabled) {
        body = Colour(0xff18201c);
        panel = Colour(0xff232d27);
        line = Colour(0xff354238);
        ink = Colour(0xffe0e7d8);
        muted = Colour(0xff9aa99a);
        green = Colour(0xffb0c9a5);
        mint = Colour(0xffbfd6b3);
        screen = Colour(0xff101a15);
        screenLine = Colour(0xff35483b);
        screenText = Colour(0xffe3eadc);
    } else {
        body = Colour(0xffe8e9e2);
        panel = Colour(0xfff4f3ed);
        line = Colour(0xffced2c9);
        ink = Colour(0xff283632);
        muted = Colour(0xff667268);
        green = Colour(0xff406f5d);
        mint = Colour(0xffbdd8bb);
        screen = Colour(0xff243430);
        screenLine = Colour(0xff41514b);
        screenText = Colour(0xffe6ece1);
    }
}
LogisthLook::LogisthLook() {
    regular = embeddedFace(false);
    semibold = embeddedFace(true);
    setDarkTheme(false);
}
void LogisthLook::setDarkTheme(bool enabled) {
    choosePalette(enabled);
    setColour(TextEditor::highlightedTextColourId, darkTheme ? screen : ink);
    setColour(ComboBox::textColourId, ink);
    setColour(TextButton::buttonColourId, darkTheme ? Colour(0xff30443b) : Colour(0xffe9ede5));
    setColour(TextButton::textColourOffId, ink);
    setColour(TextButton::buttonOnColourId, green);
    setColour(TextButton::textColourOnId, darkTheme ? screen : Colours::white);
    setColour(PopupMenu::backgroundColourId, screen);
    setColour(PopupMenu::textColourId, screenText);
    setColour(PopupMenu::highlightedBackgroundColourId, green);
    setColour(PopupMenu::highlightedTextColourId, darkTheme ? screen : Colours::white);
    setColour(TooltipWindow::backgroundColourId, screen);
    setColour(TooltipWindow::textColourId, screenText);
    setColour(TooltipWindow::outlineColourId, screenLine);
    setColour(TextEditor::backgroundColourId, panel);
    setColour(TextEditor::textColourId, ink);
    setColour(TextEditor::highlightColourId, mint);
    setColour(TextEditor::focusedOutlineColourId, green);
}
Typeface::Ptr LogisthLook::getTypefaceForFont(const Font &f) {
    return f.isBold() ? semibold : regular;
}
Font LogisthLook::getPopupMenuFont() {
    return font(12);
}
PopupMenu::Options LogisthLook::getOptionsForComboBoxPopupMenu(ComboBox &box, Label &label) {
    auto options =
        LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label).withStandardItemHeight(24);
    if (bool(box.getProperties()["routing"]))
        return options.withMinimumNumColumns(3).withMaximumNumColumns(3).withMinimumWidth(420);
    return options;
}
void LogisthLook::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float pos, float start,
                                   float end, Slider &s) {
    const bool small = bool(s.getProperties()["trim"]), isDark = (dark(s) || darkTheme);
    float radius = jmin(float(w), float(h)) * .5f - 3.f;
    if (radius <= 0)
        return;
    auto centre = Rectangle<float>(float(x), float(y), float(w), float(h)).getCentre();
    float angle = start + pos * (end - start);
    const Colour accent = isDark ? mint : green;
    Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    g.setColour(isDark ? screenLine : line);
    g.strokePath(track, PathStrokeType(small ? 1.5f : 2.f));
    const double neutral =
        s.getMinimum() < 0 && s.getMaximum() > 0 ? s.valueToProportionOfLength(0) : 0;
    float from = start + float(neutral) * (end - start);
    Path active;
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, jmin(from, angle),
                         jmax(from, angle), true);
    g.setColour(s.isEnabled() ? accent : accent.withAlpha(.25f));
    g.strokePath(active, PathStrokeType(small ? 1.6f : 2.4f));
    float r = radius - (small ? 3 : 4);
    auto knob = Rectangle<float>(centre.x - r, centre.y - r, 2 * r, 2 * r);
    if (!small) {
        g.setColour(Colours::black.withAlpha(isDark ? .12f : .06f));
        g.fillEllipse(knob.translated(0, 2));
    }
    g.setGradientFill(ColourGradient(isDark ? Colour(0xff41534a) : Colours::white, centre.x,
                                     knob.getY(), isDark ? Colour(0xff2e4038) : Colour(0xffdedfd7),
                                     centre.x, knob.getBottom(), false));
    g.fillEllipse(knob);
    g.setColour(isDark ? screenLine : Colour(0xffc4cbbf));
    g.drawEllipse(knob.reduced(.5f), 1);
    float a = angle - MathConstants<float>::halfPi;
    g.setColour(s.isEnabled() ? (isDark ? screenText : ink) : muted.withAlpha(.4f));
    g.drawLine(centre.x + std::cos(a) * r * .35f, centre.y + std::sin(a) * r * .35f,
               centre.x + std::cos(a) * r * .78f, centre.y + std::sin(a) * r * .78f,
               small ? 1.3f : 2.f);
}
Slider::SliderLayout LogisthLook::getSliderLayout(Slider &s) {
    Slider::SliderLayout layout;
    auto b = s.getLocalBounds();
    if (bool(s.getProperties()["number"])) {
        layout.sliderBounds = b;
        layout.textBoxBounds = b;
    } else if (bool(s.getProperties()["trim"]))
        layout.sliderBounds = b;
    else {
        layout.textBoxBounds = b.removeFromBottom(15);
        layout.sliderBounds = b;
    }
    return layout;
}
Label *LogisthLook::createSliderTextBox(Slider &s) {
    auto *box = LookAndFeel_V4::createSliderTextBox(s);
    box->setFont(font(bool(s.getProperties()["number"]) ? 11 : 10));
    box->setColour(Label::textColourId, (dark(s) || darkTheme) ? screenText : ink);
    box->setColour(Label::backgroundColourId, Colours::transparentBlack);
    box->setColour(Label::outlineColourId, Colours::transparentBlack);
    box->setBorderSize(BorderSize<int>(0));
    return box;
}
void LogisthLook::drawLinearSlider(Graphics &g, int x, int y, int w, int h, float pos, float, float,
                                   Slider::SliderStyle, Slider &s) {
    bool isDark = (dark(s) || darkTheme);
    auto b = Rectangle<float>(float(x), float(y), float(w), float(h));
    if (bool(s.getProperties()["number"])) {
        capsule(g, b, isDark ? Colour(0xff30443b) : Colour(0xffe9ede5), isDark ? screenLine : line);
        float p = float(s.valueToProportionOfLength(s.getValue()));
        g.setColour((isDark ? mint : green).withAlpha(.4f));
        g.fillRoundedRectangle(b.getX() + 5, b.getBottom() - 2, jmax(0.f, (b.getWidth() - 10) * p),
                               1, 0.5f);
        return;
    }
    const float cx = b.getCentreX(), top = b.getY() + 4, bottom = b.getBottom() - 4;
    g.setColour(isDark ? screenLine : line);
    g.fillRoundedRectangle(cx - 2, top, 4, bottom - top, 2);
    const float py = jlimit(top, bottom, pos);
    g.setColour((isDark ? mint : green).withAlpha(s.isEnabled() ? .65f : .2f));
    g.fillRoundedRectangle(cx - 2, py, 4, jmax(0.f, bottom - py), 2);
    g.setColour(isDark ? screenText : Colour(0xfffcfcf8));
    g.fillRoundedRectangle(cx - 9, py - 4, 18, 8, 2);
    g.setColour(isDark ? screenLine : Colour(0xffa3b0a2));
    g.drawRoundedRectangle(cx - 9, py - 4, 18, 8, 2, 1);
    g.setColour(isDark ? green : muted);
    g.drawHorizontalLine(int(py), cx - 6, cx + 6);
}
void LogisthLook::drawComboBox(Graphics &g, int w, int h, bool, int, int, int, int, ComboBox &c) {
    bool isDark = (dark(c) || darkTheme);
    capsule(g, Rectangle<float>(0, 0, float(w), float(h)),
            isDark ? Colour(0xff30443b) : Colour(0xffe9ede5), isDark ? screenLine : line);
    Path arrow;
    float cx = float(w) - 10, cy = float(h) * .5f;
    arrow.startNewSubPath(cx - 3, cy - 1);
    arrow.lineTo(cx, cy + 2);
    arrow.lineTo(cx + 3, cy - 1);
    g.setColour((isDark ? mint : green).withAlpha(c.isEnabled() ? .9f : .3f));
    g.strokePath(arrow, PathStrokeType(1.2f));
}
Font LogisthLook::getComboBoxFont(ComboBox &c) {
    return font(
        float(c.getProperties().getWithDefault("fontSize", c.getWidth() < 65 ? 9.5f : 11.f)));
}
void LogisthLook::positionComboBoxText(ComboBox &c, Label &l) {
    l.setBounds(7, 0, jmax(0, c.getWidth() - 24), c.getHeight());
    l.setFont(getComboBoxFont(c));
    l.setColour(Label::textColourId, (dark(c) || darkTheme) ? screenText : ink);
    l.setBorderSize(BorderSize<int>(0));
}
void LogisthLook::drawToggleButton(Graphics &g, ToggleButton &b, bool hover, bool) {
    const bool on = b.getToggleState(), isDark = (dark(b) || darkTheme);
    auto bounds = b.getLocalBounds().toFloat();
    if (b.getWidth() <= 22) {
        g.setColour(on ? mint : screenLine);
        g.fillRoundedRectangle(bounds.reduced(6), 2);
        return;
    }
    capsule(g, bounds,
            on ? (isDark ? mint : green) : (isDark ? Colour(0xff30443b) : Colour(0xffe9ede5)),
            on ? green : (isDark ? screenLine : line));
    if (hover) {
        g.setColour(Colours::white.withAlpha(.06f));
        g.fillRoundedRectangle(bounds, 4);
    }
    drawCaption(g, b.getButtonText(), b.getLocalBounds().reduced(2), 9,
                on ? (isDark ? screen : Colours::white) : (isDark ? screenText : muted),
                Justification::centred, true);
}
void LogisthLook::drawCornerResizer(Graphics &g, int w, int h, bool, bool) {
    g.setColour(muted.withAlpha(.45f));
    for (int i = 1; i < 4; ++i)
        g.drawLine(float(w - i * 4), float(h - 2), float(w - 2), float(h - i * 4), 1);
}

class LogisthEditor::Console : public Component, private Timer, public LogisthColours {
    enum class Style { Knob, Trim, Fader, Number, Menu, Toggle };
    class Control : public Component {
      public:
        Control(Console &o, Param index)
            : owner(o), spec(parameterSpecs[size_t(index)]), id(index) {
            setLookAndFeel(&owner.look);
            setComponentID(spec.id);
            addAndMakeVisible(label);
            addAndMakeVisible(slider);
            addAndMakeVisible(menu);
            addAndMakeVisible(toggle);
            label.setJustificationType(Justification::centred);
            label.setFont(font(9.8f));
            label.setInterceptsMouseClicks(false, false);
            const String tooltip = String(spec.group) + " / " + spec.label;
            slider.setName(tooltip);
            menu.setName(tooltip);
            toggle.setName(tooltip);
            slider.setTooltip(tooltip);
            menu.setTooltip(tooltip);
            toggle.setTooltip(tooltip);
            setTitle(tooltip);
            setDescription(tooltip);
            if (*spec.choices) {
                auto choices = StringArray::fromTokens(spec.choices, "|", "");
                if (choices == StringArray{"Off", "On"}) {
                    kind = Style::Toggle;
                    toggle.setButtonText(spec.label);
                    toggleAttachment =
                        std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
                            owner.processor.state, spec.id, toggle);
                } else {
                    kind = Style::Menu;
                    menu.getProperties().set("routing", String(spec.id).contains("dest"));
                    for (int i = 0; i < choices.size(); ++i) {
                        if (String(spec.id).contains("dest") && (i == 13 || i == 19))
                            menu.getRootMenu()->addColumnBreak();
                        if (String(spec.id).contains("dest") && (i == 1 || i == 13 || i == 19))
                            menu.getRootMenu()->addSectionHeader(i == 1 ? "Oscillators"
                                                                 : i == 13
                                                                     ? "Filters"
                                                                     : "Modulation / effects");
                        String name = choices[i];
                        if (String(spec.id).endsWith("slope"))
                            name = i == 0 ? "12 dB" : "24 dB";
                        if (String(spec.id).contains("dest")) {
                            name = name.replace(" and B", "+B")
                                       .replace("Resonance", "Reso")
                                       .replace("Phaser Frequency", "Phaser freq")
                                       .replace("Distortion Amount", "Drive amount");
                        }
                        menu.addItem(name, i + 1);
                    }
                    choiceAttachment =
                        std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
                            owner.processor.state, spec.id, menu);
                }
            } else {
                kind = Style::Knob;
                sliderAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                    owner.processor.state, spec.id, slider);
                slider.textFromValueFunction = [this](double v) { return formatted(v); };
                slider.valueFromTextFunction = [this](const String &t) {
                    double v = t.getDoubleValue();
                    if (t.containsIgnoreCase("k"))
                        v *= 1000;
                    const bool storedMilliseconds = String(spec.label).contains("/ ms");
                    if (t.containsIgnoreCase("ms") && !storedMilliseconds)
                        v *= .001;
                    else if (t.endsWithIgnoreCase("s") && !t.endsWithIgnoreCase("ms") &&
                             storedMilliseconds)
                        v *= 1000;
                    if (percent())
                        v *= .01;
                    return v;
                };
                slider.setDoubleClickReturnValue(
                    true, String(spec.id).contains("amt") ? 0 : spec.initial);
                slider.onValueChange = [this] {
                    if (isMouseOverOrDragging(true))
                        describe();
                    if (String(spec.id).contains("amp"))
                        owner.repaint(500, 113, 240, 28);
                };
            }
            menu.onChange = [this] {
                if (isMouseOverOrDragging(true))
                    describe();
            };
            toggle.onClick = [this] { describe(); };
            addMouseListener(this, true);
        }
        ~Control() override {
            removeMouseListener(this);
            setLookAndFeel(nullptr);
        }
        void configure(Rectangle<int> b, Style style, const String &title, bool isDark = false) {
            kind = style;
            caption = title;
            getProperties().set("dark", isDark);
            setBounds(b);
            setVisible(true);
            label.setText(title, dontSendNotification);
            label.setColour(Label::textColourId, isDark ? owner.mint.withAlpha(.7f) : owner.muted);
            slider.getProperties().set("number", kind == Style::Number);
            slider.getProperties().set("trim", kind == Style::Trim);
            slider.setSliderStyle(kind == Style::Fader    ? Slider::LinearVertical
                                  : kind == Style::Number ? Slider::LinearBar
                                                          : Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(kind == Style::Trim     ? Slider::NoTextBox
                                   : kind == Style::Number ? Slider::TextBoxLeft
                                                           : Slider::TextBoxBelow,
                                   false, b.getWidth(), kind == Style::Number ? b.getHeight() : 15);
            toggle.setButtonText(title);
            slider.updateText();
            resized();
        }
        void resized() override {
            bool heading = caption.isNotEmpty() && kind != Style::Toggle && kind != Style::Trim;
            label.setVisible(heading);
            auto b = getLocalBounds();
            if (heading)
                label.setBounds(b.removeFromTop(13));
            slider.setVisible(kind == Style::Knob || kind == Style::Trim || kind == Style::Fader ||
                              kind == Style::Number);
            menu.setVisible(kind == Style::Menu);
            toggle.setVisible(kind == Style::Toggle);
            slider.setBounds(b);
            menu.setBounds(b.withSizeKeepingCentre(b.getWidth(), jmin(25, b.getHeight())));
            toggle.setBounds(b);
        }
        void mouseEnter(const MouseEvent &) override {
            describe();
        }
        void describe() {
            owner.readout = String(spec.group) + "  /  " + String(spec.label) + "   " +
                            owner.processor.state.getParameter(spec.id)->getCurrentValueAsText();
            owner.repaint(118, 844, 870, 16);
        }
        Param parameter() const {
            return id;
        }
        void enable(bool enabled) {
            setEnabled(enabled);
            label.setAlpha(enabled ? 1.f : .4f);
        }
        void selectMenuItem(int item) {
            menu.setSelectedId(item, sendNotificationSync);
        }
        double sliderValue() const {
            return slider.getValue();
        }

      private:
        bool percent() const {
            return spec.low >= 0 && spec.high <= 1 && String(spec.id) != "portamento";
        }
        String formatted(double v) const {
            const String name(spec.id);
            if (name.endsWith("fine"))
                return String(v, 1) + "ct";
            if (String(spec.label).contains("/ ms"))
                return String(v, std::abs(v) >= 10 ? 0 : 1) + "ms";
            if (String(spec.label).contains("/ dB"))
                return String(v, 0) + "dB";
            if (name == "compratio")
                return String(v, 1) + ":1";
            if (name.contains("velocity"))
                return v < .5 ? String(CharPointer_UTF8("—")) : String(int(v));
            if (name == "filtercutoff")
                return String(v, 0) + "st";
            if (name == "p1cutoff" || name == "p2cutoff" || name.contains("freq") ||
                name.contains("Freq") || name == "delaylowcut" || name == "delayhighcut")
                return v >= 1000 ? String(v / 1000, 1) + "k" : String(int(v));
            if (name.endsWith("ampa") || name.endsWith("ampd") || name.endsWith("ampr") ||
                name == "env1a" || name == "env1d" || name == "env1r" || name == "env2a" ||
                name == "env2d" || name == "env2r" || name == "portamento")
                return v < 1 ? String(v * 1000, 0) + "ms" : String(v, 1) + "s";
            if (percent())
                return String(v * 100, 0) + "%";
            if (spec.integer)
                return (v > 0 && spec.low < 0 ? "+" : "") + String(int(v));
            if (name.endsWith("phase"))
                return String(v, 0) + String(CharPointer_UTF8("°"));
            return String(v, std::abs(v) >= 10 ? 0 : 2);
        }
        Console &owner;
        const ParameterSpec &spec;
        Param id;
        Style kind = Style::Knob;
        String caption;
        Label label;
        Slider slider;
        ComboBox menu;
        ToggleButton toggle;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
        std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
    };
    class RackButton : public Button {
      public:
        RackButton(Console &c, int i, String n) : Button(n), owner(c), index(i), name(n) {
            onClick = [this] { owner.setRack(index); };
        }
        void paintButton(Graphics &g, bool hover, bool) override {
            bool selected = owner.rack == index;
            g.setColour(selected ? Colour(0xff3d5146)
                        : hover  ? Colour(0xff2d4037)
                                 : Colours::transparentBlack);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
            drawCaption(g, name, getLocalBounds().withTrimmedLeft(21), 10,
                        selected ? owner.screenText : owner.mint.withAlpha(.66f),
                        Justification::centredLeft, selected);
            if (selected) {
                g.setColour(owner.mint);
                g.fillRoundedRectangle(0, 5, 2, float(getHeight() - 10), 1);
            }
        }

      private:
        Console &owner;
        int index;
        String name;
    };
    class Keys : public MidiKeyboardComponent {
      public:
        Keys(MidiKeyboardState &s, Console &c)
            : MidiKeyboardComponent(s, horizontalKeyboard), owner(c) {
            applyTheme();
        }
        void applyTheme() {
            setColour(whiteNoteColourId, owner.darkTheme ? Colour(0xffb9c5b7) : Colour(0xfff8f7f1));
            setColour(blackNoteColourId, owner.darkTheme ? Colour(0xff1d2822) : owner.ink);
            setColour(keySeparatorLineColourId, owner.line);
            setColour(mouseOverKeyOverlayColourId, owner.mint.withAlpha(.4f));
            setColour(keyDownOverlayColourId, owner.green.withAlpha(.55f));
            setColour(textLabelColourId, owner.darkTheme ? Colour(0xff435347) : owner.muted);
            setColour(shadowColourId, Colours::transparentBlack);
        }
        void drawBlackNote(int, Graphics &g, Rectangle<float> b, bool down, bool over,
                           Colour) override {
            g.setColour(down              ? owner.green
                        : over            ? Colour(0xff48574d)
                        : owner.darkTheme ? Colour(0xff1d2822)
                                          : owner.ink);
            g.fillRoundedRectangle(b.withTrimmedBottom(4), 2);
            g.setColour(Colours::white.withAlpha(.08f));
            g.fillRoundedRectangle(b.reduced(2, 1).withHeight(b.getHeight() - 9), 1);
        }

      private:
        Console &owner;
    };

  public:
    Console(LogisthProcessor &p, LogisthLook &skin)
        : processor(p), look(skin), keyboard(p.keyboard, *this), tooltips(this, 650) {
        setLookAndFeel(&look);
        setSize(canvasWidth, canvasHeight);
        for (size_t i = 0; i < parameterCount; ++i) {
            controls[i] = std::make_unique<Control>(*this, Param(i));
            addChildComponent(*controls[i]);
        }
        for (int i = 0; i < 2; ++i) {
            auto &button = parts[size_t(i)];
            button.setButtonText(i == 0 ? "A" : "B");
            button.onClick = [this, i] { setPart(i); };
            addAndMakeVisible(button);
        }
        StringArray names{"ARPEGGIATOR", "DISTORTION", "PHASER", "CHORUS",
                          "EQUALIZER",   "REVERB",     "DELAY",  "COMPRESSOR"};
        for (int i = 0; i < 8; ++i) {
            rackButtons[size_t(i)] = std::make_unique<RackButton>(*this, i, names[i]);
            addAndMakeVisible(*rackButtons[size_t(i)]);
        }
        for (int i = 0; i < 2; ++i) {
            stepPages[size_t(i)].setButtonText(i == 0 ? "01-08" : "09-16");
            stepPages[size_t(i)].onClick = [this, i] {
                stepPage = i;
                layout();
            };
            stepPages[size_t(i)].getProperties().set("dark", true);
            addChildComponent(stepPages[size_t(i)]);
        }
        presets.addItemList({"Init / analog", "Wide supersaw", "Sub bass", "Slow atmosphere",
                             "Pulse pluck", "Step sequence"},
                            1);
        presets.setTextWhenNothingSelected("Init / custom patch");
        presets.getProperties().set("fontSize", 12.f);
        presets.onChange = [this] {
            processor.loadPreset(presets.getSelectedId() - 1);
            readout = "Factory sound  /  " + presets.getText();
            repaint();
        };
        addAndMakeVisible(presets);
        previous.setButtonText(String(CharPointer_UTF8("‹")));
        next.setButtonText(String(CharPointer_UTF8("›")));
        previous.onClick = [this] {
            int i = presets.getSelectedId();
            presets.setSelectedId(i <= 1 ? 6 : i - 1, sendNotificationSync);
        };
        next.onClick = [this] {
            int i = presets.getSelectedId();
            presets.setSelectedId(i < 1 || i >= 6 ? 1 : i + 1, sendNotificationSync);
        };
        addAndMakeVisible(previous);
        addAndMakeVisible(next);
        keyboard.setAvailableRange(36, 96);
        keyboard.setOctaveForMiddleC(4);
        keyboard.setKeyWidth(24);
        addAndMakeVisible(keyboard);
        for (auto *wheel : {&pitch, &mod}) {
            wheel->setSliderStyle(Slider::LinearVertical);
            wheel->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            wheel->getProperties().set("trim", true);
            addAndMakeVisible(wheel);
        }
        pitch.setRange(-1, 1, .001);
        pitch.setValue(0);
        mod.setRange(0, 1, .001);
        pitch.setTooltip("Pitch bend");
        mod.setTooltip("Modulation wheel");
        pitch.onValueChange = [this] { processor.setUiWheel(true, float(pitch.getValue())); };
        pitch.onDragEnd = [this] { pitch.setValue(0); };
        mod.onValueChange = [this] { processor.setUiWheel(false, float(mod.getValue())); };
        theme.setComponentID("ui.theme");
        theme.setName("Colour theme");
        theme.setTooltip("Light / dark appearance");
        theme.onClick = [this] { setDarkTheme(theme.getToggleState()); };
        addAndMakeVisible(theme);
        readout = "Ready  /  Choose a sound or play a note";
        setDarkTheme(int(processor.state.state.getProperty("uiDark", 0)) != 0, false);
        startTimerHz(24);
    }
    ~Console() override {
        stopTimer();
        for (auto &c : controls)
            c.reset();
        setLookAndFeel(nullptr);
    }
    void setDarkTheme(bool enabled, bool persist = true) {
        choosePalette(enabled);
        look.setDarkTheme(enabled);
        theme.setToggleState(enabled, dontSendNotification);
        theme.setButtonText(enabled ? "DARK" : "LIGHT");
        keyboard.applyTheme();
        sendLookAndFeelChange();
        layout();
        if (persist)
            processor.state.state.setProperty("uiDark", enabled, nullptr);
        repaint();
    }
    bool isDarkTheme() const {
        return darkTheme;
    }
    void setPart(int part) {
        selectedPart = jlimit(0, 1, part);
        layout();
        repaint();
    }
    void setRack(int page) {
        rack = jlimit(0, 7, page);
        layout();
        repaint();
    }
    void showScenario(int index) {
        selectedPart = index == 1 ? 1 : 0;
        rack = index == 1 ? 6 : index == 2 ? 2 : 0;
        stepPage = index == 3 ? 1 : 0;
        layout();
        repaint();
    }
    String validate() {
        const int oldPart = selectedPart, oldRack = rack, oldStep = stepPage;
        std::set<size_t> visited;
        String error;
        for (int clock = 0; clock < 2; ++clock) {
            previewSync = clock;
            for (int part = 0; part < 2; ++part)
                for (int page = 0; page < 8; ++page)
                    for (int step = 0; step < 2; ++step) {
                        selectedPart = part;
                        rack = page;
                        stepPage = step;
                        layout();
                        for (size_t i = 0; i < controls.size(); ++i)
                            if (controls[i]->isVisible()) {
                                visited.insert(i);
                                auto *hit = getComponentAt(controls[i]->getBounds().getCentre());
                                if (hit != controls[i].get() && !controls[i]->isParentOf(hit))
                                    error = "Control obscured by another component: " +
                                            String(parameterSpecs[i].id);
                                if (!getLocalBounds().contains(controls[i]->getBounds()) ||
                                    controls[i]->getWidth() < 20 || controls[i]->getHeight() < 18)
                                    error =
                                        "Invalid control bounds: " + String(parameterSpecs[i].id);
                                for (size_t j = 0; j < i; ++j)
                                    if (controls[j]->isVisible() &&
                                        controls[i]->getBounds().intersects(
                                            controls[j]->getBounds()))
                                        error = "Overlapping controls: " +
                                                String(parameterSpecs[i].id) + " / " +
                                                parameterSpecs[j].id;
                            }
                    }
        }
        previewSync = -1;
        for (size_t i = 0; i < controls.size(); ++i)
            if (visited.count(i) == 0)
                error = "Unreachable parameter: " + String(parameterSpecs[i].id);
        selectedPart = oldPart;
        rack = oldRack;
        stepPage = oldStep;
        layout();
        return error;
    }

  private:
    friend class Control;
    void shell(Graphics &g, Rectangle<int> b, const String &title, const String &hint = {}) {
        paintShell(g, b, title, hint, *this);
    }
    static constexpr std::array<Param, 8> enables{Param::arpon,    Param::diston, Param::phaseron,
                                                  Param::choruson, Param::eqon,   Param::reverbon,
                                                  Param::delayon,  Param::compon};
    float value(Param p) const {
        if (previewSync >= 0) {
            if (p == Param::sync)
                return float(previewSync);
            if (p == Param::lfo1free || p == Param::lfo2free)
                return 0;
        }
        return processor.state.getRawParameterValue(parameterSpecs[size_t(p)].id)->load();
    }
    void put(Param id, Rectangle<int> bounds, Style style, const String &label = {},
             bool isDark = false) {
        controls[size_t(id)]->configure(bounds, style, label, isDark);
    }
    void control(const String &id, Rectangle<int> b, Style style, const String &title = {},
                 bool isDark = false) {
        for (size_t i = 0; i < parameterCount; ++i)
            if (id == parameterSpecs[i].id) {
                put(Param(i), b, style, title, isDark);
                return;
            }
        jassertfalse;
    }
    void oscillator(int number, Rectangle<int> b) {
        String id = "o" + String(number);
        StringArray names{"VOLUME", "PHASE", "DETUNE", "STEREO", "PAN"};
        StringArray suffix{"level", "phase", "detune", "stereo", "pan"};
        for (int i = 0; i < 3; ++i)
            control(id + StringArray{"octave", "tune", "fine"}[i],
                    {b.getX() + 12, b.getY() + 34 + i * 39, 62, 33}, Style::Number,
                    StringArray{"OCTAVE", "NOTE", "FINE"}[i]);
        for (int i = 0; i < 5; ++i)
            control(id + suffix[i], {b.getX() + 86 + i * 72, b.getY() + 35, 66, 86}, Style::Knob,
                    names[i]);
        control(id + "wave", {b.getX() + 87, b.getY() + 137, 120, 26}, Style::Menu);
        control(id + "voices", {b.getX() + 217, b.getY() + 130, 58, 38}, Style::Number, "VOICES");
        control(id + "retrig", {b.getX() + 287, b.getY() + 141, 74, 22}, Style::Toggle, "RETRIG");
        control(id + "invert", {b.getX() + 371, b.getY() + 141, 74, 22}, Style::Toggle, "INVERT");
        controls[size_t(Param(size_t(Param::o1phase) + size_t(number - 1) * 12))]->enable(
            value(Param(size_t(Param::o1retrig) + size_t(number - 1) * 12)) > 0);
    }
    void envelope(const String &base, Rectangle<int> b, bool mini) {
        const int w = mini ? 29 : 48, gap = mini ? 6 : 12, left = mini ? 10 : 19,
                  top = mini ? 31 : 59, height = mini ? 83 : 105;
        for (int i = 0; i < 4; ++i)
            control(base + StringArray{"a", "d", "s", "r"}[i],
                    {b.getX() + left + i * (w + gap), b.getY() + top, w, height}, Style::Fader,
                    StringArray{"A", "D", "S", "R"}[i]);
    }
    void routes(const String &base, Rectangle<int> b, int y, int row, bool isDark = false) {
        for (int i = 1; i <= 2; ++i) {
            control(base + "amt" + String(i), {b.getX() + 8, b.getY() + y + (i - 1) * row, 30, 30},
                    Style::Trim, {}, isDark);
            control(base + "dest" + String(i),
                    {b.getX() + 43, b.getY() + y + 3 + (i - 1) * row, b.getWidth() - 53, 24},
                    Style::Menu, {}, isDark);
        }
    }
    void lfo(int index, Rectangle<int> b) {
        String id = "lfo" + String(index);
        control(id + "wave", {b.getX() + 10, b.getY() + 30, 111, 23}, Style::Menu);
        control(id + "free", {b.getX() + 130, b.getY() + 30, 59, 23}, Style::Toggle, "FREE");
        bool clock =
            value(Param::sync) > 0 && value(index == 1 ? Param::lfo1free : Param::lfo2free) == 0;
        control(id + (clock ? "division" : "rate"), {b.getX() + 10, b.getY() + 60, 56, 66},
                clock ? Style::Menu : Style::Knob, "RATE");
        control(id + "gain", {b.getX() + 72, b.getY() + 60, 56, 66}, Style::Knob, "GAIN");
        control(id + "offset", {b.getX() + 134, b.getY() + 60, 56, 66}, Style::Knob, "OFFSET");
        routes(id, b, 133, 35);
    }
    void misc(int first, Rectangle<int> b) {
        for (int source = 0; source < 2; ++source) {
            String id = "misc" + String(first + source);
            int y = source == 0 ? 28 : 113;
            control(id + "source", {b.getX() + 10, b.getY() + y, b.getWidth() - 20, 23},
                    Style::Menu);
            for (int i = 1; i <= 2; ++i) {
                control(id + "amt" + String(i),
                        {b.getX() + 8, b.getY() + y + 26 + (i - 1) * 27, 28, 27}, Style::Trim);
                control(id + "dest" + String(i),
                        {b.getX() + 43, b.getY() + y + 28 + (i - 1) * 27, b.getWidth() - 53, 23},
                        Style::Menu);
            }
        }
    }
    void rackLayout() {
        Rectangle<int> content{414, 320, 520, 203};
        for (int i = 0; i < 8; ++i) {
            rackButtons[size_t(i)]->setBounds(291, 315 + i * 27, 108, 25);
            put(enables[size_t(i)], {294, 318 + i * 27, 20, 20}, Style::Toggle, {}, true);
            controls[size_t(enables[size_t(i)])]->toFront(false);
        }
        if (rack == 0) {
            control("arpmode", {content.getX(), content.getY(), 128, 38}, Style::Menu, "MODE",
                    true);
            control("arpvelocity", {content.getX(), content.getY() + 43, 128, 38}, Style::Menu,
                    "VELOCITY", true);
            bool sync = value(Param::sync) > 0;
            put(sync ? Param::arpdivision : Param::arptime,
                {content.getX() + 140, content.getY(), 72, 80}, sync ? Style::Menu : Style::Knob,
                "TIME", true);
            put(Param::arpgate, {content.getX() + 222, content.getY(), 62, 80}, Style::Knob, "GATE",
                true);
            put(Param::arpswing, {content.getX() + 290, content.getY(), 62, 80}, Style::Knob,
                "SWING", true);
            put(Param::arpoctaves, {content.getX() + 365, content.getY() + 3, 44, 36},
                Style::Number, "OCT", true);
            put(Param::arpwrap, {content.getX() + 418, content.getY() + 3, 44, 36}, Style::Number,
                "WRAP", true);
            put(Param::arpsteps, {content.getX() + 471, content.getY() + 3, 44, 36}, Style::Number,
                "LENGTH", true);
            for (int i = 0; i < 2; ++i) {
                stepPages[size_t(i)].setBounds(content.getX() + 365 + i * 77, content.getY() + 55,
                                               70, 22);
                stepPages[size_t(i)].setToggleState(stepPage == i, dontSendNotification);
                stepPages[size_t(i)].setVisible(true);
            }
            for (int i = 0; i < 8; ++i) {
                int step = stepPage * 8 + i + 1, x = content.getX() + 42 + i * 59;
                String base = "step" + String(step);
                control(base + "transpose", {x + 3, content.getY() + 108, 50, 23}, Style::Number,
                        {}, true);
                control(base + "velocity", {x + 3, content.getY() + 136, 50, 23}, Style::Number, {},
                        true);
                control(base + "hold", {x + 6, content.getY() + 165, 44, 18}, Style::Toggle, "HOLD",
                        true);
                controls[size_t(Param(size_t(Param::step1transpose) + size_t(step - 1) * 3))]
                    ->enable(value(Param::arpmode) >= 8);
            }
        } else {
            const String group =
                StringArray{"ARPEGGIATOR", "DISTORTION", "PHASER", "CHORUS",
                            "EQUALIZER",   "REVERB",     "DELAY",  "COMPRESSOR"}[rack];
            std::vector<size_t> list;
            bool sync = value(Param::sync) > 0;
            for (size_t i = 0; i < parameterCount; ++i) {
                auto &s = parameterSpecs[i];
                String id(s.id);
                if (group != s.group || Param(i) == enables[size_t(rack)])
                    continue;
                bool seconds = id == "phaserrate" || id == "chorusrate" || id == "delaytime" ||
                               id == "delayright";
                bool division = id == "phaserdivision" || id == "chorusdivision" ||
                                id == "delaydivL" || id == "delaydivR";
                if ((seconds && sync) || (division && !sync))
                    continue;
                list.push_back(i);
            }
            const int columns = list.size() <= 6 ? int(list.size()) : int((list.size() + 1) / 2);
            int rows = int((list.size() + size_t(columns) - 1) / size_t(columns));
            int cellWidth = 520 / columns;
            for (size_t k = 0; k < list.size(); ++k) {
                auto i = list[k];
                auto &s = parameterSpecs[i];
                String label = String(s.label)
                                   .replace(" / Hz", "")
                                   .replace(" / ms", "")
                                   .replace(" / dB", "")
                                   .replace(" / s", "")
                                   .replace("Sync division", "RATE")
                                   .replace("LFO rate", "RATE")
                                   .toUpperCase();
                if (String(s.id) == "delaydivL")
                    label = "LEFT";
                if (String(s.id) == "delaydivR")
                    label = "RIGHT";
                Style style = *s.choices
                                  ? (String(s.choices) == "Off|On" ? Style::Toggle : Style::Menu)
                                  : Style::Knob;
                int h = rows == 1 ? 117 : 89;
                auto cell = Rectangle<int>(
                    content.getX() + int(k % size_t(columns)) * cellWidth + 4,
                    content.getY() + (rows == 1 ? 20 : 0) + int(k / size_t(columns)) * 100,
                    cellWidth - 8, h);
                if (style == Style::Toggle)
                    cell = cell.withSizeKeepingCentre(cell.getWidth(), 24);
                put(Param(i), cell, style, label, true);
            }
        }
    }
    void layout() {
        for (auto &c : controls) {
            c->setVisible(false);
            c->enable(true);
        }
        for (auto &button : stepPages)
            button.setVisible(false);
        presets.setBounds(364, 25, 362, 32);
        previous.setBounds(332, 25, 26, 32);
        next.setBounds(732, 25, 26, 32);
        for (int i = 0; i < 2; ++i) {
            parts[size_t(i)].setBounds(834 + i * 40, 28, 34, 24);
            parts[size_t(i)].setToggleState(i == selectedPart, dontSendNotification);
        }
        put(Param::solo, {927, 28, 68, 24}, Style::Menu);
        put(Param::polyphony, {1010, 16, 60, 38}, Style::Number, "POLY");
        put(Param::sync, {1085, 28, 64, 24}, Style::Toggle, "SYNC");
        theme.setBounds(1162, 28, 62, 24);
        const int first = selectedPart * 2 + 1;
        oscillator(first, {16, 82, 460, 184});
        oscillator(first + 1, {764, 82, 460, 184});
        envelope("p" + String(selectedPart + 1) + "amp", {486, 82, 268, 184}, false);
        String filter = "p" + String(selectedPart + 1);
        control(filter + "input", {28, 311, 60, 24}, Style::Menu);
        control(filter + "type", {94, 311, 100, 24}, Style::Menu);
        control(filter + "slope", {200, 311, 56, 24}, Style::Menu);
        for (int i = 0; i < 3; ++i)
            control(filter + StringArray{"cutoff", "res", "drive"}[i], {28 + i * 76, 341, 70, 72},
                    Style::Knob, StringArray{"CUTOFF", "RESO", "DRIVE"}[i]);
        put(Param::filtercutoff, {27, 462, 57, 66}, Style::Knob, "CUTOFF");
        put(Param::filterres, {89, 462, 57, 66}, Style::Knob, "RESO");
        put(Param::keytrack, {151, 462, 57, 66}, Style::Knob, "KEYTRACK");
        put(Param::warmdrive, {215, 479, 42, 22}, Style::Toggle, "WARM");
        put(Param::mixA, {975, 315, 45, 196}, Style::Fader, "MIX A");
        put(Param::mixB, {1031, 315, 45, 196}, Style::Fader, "MIX B");
        put(Param::master, {1087, 315, 45, 196}, Style::Fader, "MAIN");
        envelope("env1", {16, 548, 152, 208}, true);
        envelope("env2", {176, 548, 152, 208}, true);
        routes("env1", {16, 548, 152, 208}, 128, 35);
        routes("env2", {176, 548, 152, 208}, 128, 35);
        lfo(1, {336, 548, 200, 208});
        lfo(2, {544, 548, 200, 208});
        misc(1, {752, 548, 232, 208});
        misc(3, {992, 548, 232, 208});
        rackLayout();
        pitch.setBounds(29, 784, 20, 46);
        mod.setBounds(63, 784, 20, 46);
        keyboard.setBounds(118, 771, 864, 70);
        keyboard.setLowestVisibleKey(36);
        put(Param::monolegato, {1004, 779, 94, 21}, Style::Toggle, "MONO / LEGATO");
        put(Param::portamode, {1108, 779, 104, 21}, Style::Menu);
        put(Param::bendrange, {1004, 809, 48, 28}, Style::Number, "BEND / st");
        put(Param::portamento, {1126, 813, 76, 24}, Style::Number);
        parts[0].repaint();
        parts[1].repaint();
        for (auto &b : rackButtons)
            b->repaint();
        clockSignature = int(value(Param::sync)) * 8 + int(value(Param::lfo1free)) * 4 +
                         int(value(Param::lfo2free)) * 2 + int(value(Param::arpmode) >= 8);
    }
    void timerCallback() override {
        int signature = int(value(Param::sync)) * 8 + int(value(Param::lfo1free)) * 4 +
                        int(value(Param::lfo2free)) * 2 + int(value(Param::arpmode) >= 8);
        if (signature != clockSignature)
            layout();
        for (int i = 0; i < 4; ++i)
            controls[size_t(Param(size_t(Param::o1phase) + size_t(i) * 12))]->enable(
                value(Param(size_t(Param::o1retrig) + size_t(i) * 12)) > 0);
        controls[size_t(Param::polyphony)]->enable(value(Param::monolegato) == 0);
        if (!pitch.isMouseButtonDown())
            pitch.setValue(processor.pitchPosition.load(), dontSendNotification);
        if (!mod.isMouseButtonDown())
            mod.setValue(processor.wheelPosition.load(), dontSendNotification);
        repaint(956, 278, 268, 258);
    }
    void paint(Graphics &g) override {
        g.fillAll(body);
        g.setColour(darkTheme ? line : Colour(0xfff6f5ee));
        g.drawHorizontalLine(0, 0, float(canvasWidth));
        g.setColour(green);
        Path mark;
        mark.startNewSubPath(26, 30);
        mark.lineTo(31, 43);
        mark.lineTo(38, 24);
        mark.lineTo(45, 43);
        mark.lineTo(50, 30);
        g.strokePath(mark, PathStrokeType(2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        drawCaption(g, "logisth", {62, 13, 180, 38}, 31, ink, Justification::centredLeft, true);
        drawCaption(g, "POLYPHONIC INSTRUMENT", {63, 50, 220, 14}, 8.5f, muted);
        drawCaption(g, "SOUND LIBRARY", {364, 9, 280, 13}, 8, muted);
        drawCaption(g, "PART", {795, 30, 33, 20}, 8.5f, muted);
        drawCaption(g, "SOLO", {927, 12, 68, 13}, 8, muted);
        drawCaption(g, "THEME", {1162, 12, 62, 13}, 8, muted);
        String part = selectedPart == 0 ? "A" : "B";
        shell(g, {16, 82, 460, 184}, "OSCILLATOR " + part + "1", "01 / VOICE");
        shell(g, {764, 82, 460, 184}, "OSCILLATOR " + part + "2", "02 / VOICE");
        shell(g, {486, 82, 268, 184}, "AMP ENVELOPE " + part, "ADSR");
        auto curveBounds = Rectangle<float>(508, 116, 224, 22);
        size_t ampBase = size_t(Param::p1ampa) + size_t(selectedPart) * 10;
        const float attack = .05f + .45f * std::sqrt(value(Param(ampBase)) / 8.f);
        const float decay = .05f + .45f * std::sqrt(value(Param(ampBase + 1)) / 8.f);
        const float release = .05f + .45f * std::sqrt(value(Param(ampBase + 3)) / 12.f);
        const float norm = curveBounds.getWidth() / (attack + decay + release + .2f);
        const float sustainY =
            curveBounds.getBottom() - value(Param(ampBase + 2)) * curveBounds.getHeight();
        Path curve;
        curve.startNewSubPath(curveBounds.getX(), curveBounds.getBottom());
        curve.lineTo(curveBounds.getX() + attack * norm, curveBounds.getY());
        curve.lineTo(curveBounds.getX() + (attack + decay) * norm, sustainY);
        curve.lineTo(curveBounds.getRight() - release * norm, sustainY);
        curve.lineTo(curveBounds.getRight(), curveBounds.getBottom());
        g.setColour(green.withAlpha(.4f));
        g.strokePath(curve, PathStrokeType(1.2f));
        shell(g, {16, 278, 252, 144}, "FILTER " + part, "INPUT / TYPE / SLOPE");
        shell(g, {16, 434, 252, 102}, "FILTER CONTROL", "A + B");
        shell(g, {956, 278, 268, 258}, "MIXER",
              "OUT / " + String(processor.voices.load()) + " VOICES");
        for (int i = 0; i < 2; ++i) {
            int x = 1162 + i * 23;
            drawCaption(g, i == 0 ? "L" : "R", {x, 314, 15, 14}, 8, muted, Justification::centred);
            float level = processor.channelPeaks[size_t(i)].load();
            float db = 20 * std::log10(jmax(.00001f, level));
            int lit = jlimit(0, 16, int((db + 48) / 3));
            for (int j = 0; j < 16; ++j) {
                g.setColour(j < lit     ? (j > 13 ? Colour(0xffbb7c46) : green)
                            : darkTheme ? Colour(0xff354138)
                                        : Colour(0xffdee2d8));
                g.fillRoundedRectangle(float(x + 3), float(489 - j * 9), 9, 6, 1);
            }
        }
        drawCaption(g, "POST FX  /  STEREO OUTPUT", {971, 515, 238, 12}, 7.5f, muted,
                    Justification::centred);
        g.setColour(screen);
        g.fillRoundedRectangle(Rectangle<float>(280, 278, 668, 258), 8);
        g.setColour(screenLine);
        g.drawRoundedRectangle(Rectangle<float>(280.5f, 278.5f, 667, 257), 8, 1);
        g.drawVerticalLine(404, 292, 523);
        drawCaption(g, "SIGNAL RACK", {292, 290, 108, 16}, 8, mint.withAlpha(.5f));
        StringArray titles{"ARPEGGIATOR", "DISTORTION", "PHASER", "CHORUS",
                           "EQUALIZER",   "REVERB",     "DELAY",  "COMPRESSOR"};
        drawCaption(g, titles[rack], {414, 289, 350, 20}, 12, screenText,
                    Justification::centredLeft, true);
        drawCaption(g, value(Param::sync) > 0 ? "HOST SYNC" : "FREE TIME", {813, 291, 118, 16}, 8,
                    mint.withAlpha(.65f), Justification::centredRight);
        if (rack == 0) {
            for (int i = 0; i < 8; ++i) {
                int x = 459 + i * 59;
                int n = stepPage * 8 + i + 1;
                drawCaption(g, String(n).paddedLeft('0', 2), {x, 408, 50, 17}, 8,
                            mint.withAlpha(.6f), Justification::centred);
            }
            drawCaption(g, "PITCH", {414, 432, 37, 14}, 7.5f, mint.withAlpha(.65f));
            drawCaption(g, "VELOCITY", {414, 460, 37, 14}, 7.5f, mint.withAlpha(.65f));
            drawCaption(g, "TRANSPOSE  /  VELOCITY  /  HOLD", {414, 512, 520, 14}, 7,
                        mint.withAlpha(.38f), Justification::centredRight);
        } else
            drawCaption(g, "DIST  /  PHASE  /  CHORUS  /  EQ  /  REVERB  /  DELAY  /  COMP",
                        {414, 513, 520, 13}, 7, mint.withAlpha(.38f), Justification::centredRight);
        shell(g, {16, 548, 152, 208}, "MOD ENV 1");
        shell(g, {176, 548, 152, 208}, "MOD ENV 2");
        shell(g, {336, 548, 200, 208}, "LFO 1");
        shell(g, {544, 548, 200, 208}, "LFO 2");
        shell(g, {752, 548, 232, 208}, "MISC 1");
        shell(g, {992, 548, 232, 208}, "MISC 2");
        g.setColour(line);
        g.drawHorizontalLine(656, 762, 974);
        g.drawHorizontalLine(656, 1002, 1214);
        shell(g, {16, 768, 90, 76}, {});
        shell(g, {990, 768, 234, 76}, {});
        drawCaption(g, "BEND", {25, 769, 29, 13}, 7, muted, Justification::centred);
        drawCaption(g, "MOD", {60, 769, 29, 13}, 7, muted, Justification::centred);
        drawCaption(g, "GLIDE", {1060, 818, 53, 16}, 8, muted, Justification::centredRight);
        drawCaption(g, readout, {120, 846, 870, 12}, 8, muted);
        drawCaption(g, "LOGISTH  /  VIRTUAL ANALOG", {990, 846, 234, 12}, 7.5f, muted,
                    Justification::centredRight);
    }
    LogisthProcessor &processor;
    LogisthLook &look;
    std::array<std::unique_ptr<Control>, parameterCount> controls;
    std::array<ToggleButton, 2> parts, stepPages;
    std::array<std::unique_ptr<RackButton>, 8> rackButtons;
    ComboBox presets;
    ToggleButton theme;
    TextButton previous, next;
    Keys keyboard;
    Slider pitch, mod;
    TooltipWindow tooltips;
    String readout;
    int selectedPart = 0, rack = 0, stepPage = 0, clockSignature = -1, previewSync = -1;
};
LogisthEditor::LogisthEditor(LogisthProcessor &p) : AudioProcessorEditor(p) {
    setLookAndFeel(&look);
    console = std::make_unique<Console>(p, look);
    addAndMakeVisible(*console);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(double(canvasWidth) / canvasHeight);
    setResizeLimits(992, 688, 1860, 1290);
    float scale = 1.f;
    if (auto *display = Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = jlimit(.8f, 1.f,
                       jmin(float(display->userArea.getWidth() - 40) / canvasWidth,
                            float(display->userArea.getHeight() - 40) / canvasHeight));
    setSize(roundToInt(canvasWidth * scale), roundToInt(canvasHeight * scale));
}
LogisthEditor::~LogisthEditor() {
    console.reset();
    setLookAndFeel(nullptr);
}
void LogisthEditor::resized() {
    if (console) {
        console->setBounds(0, 0, canvasWidth, canvasHeight);
        console->setTransform(AffineTransform::scale(float(getWidth()) / canvasWidth,
                                                     float(getHeight()) / canvasHeight));
    }
}
Image LogisthEditor::snapshotPage(int scenario) {
    console->setDarkTheme(scenario >= 4, false);
    console->showScenario(scenario % 4);
    return createComponentSnapshot(getLocalBounds());
}
String LogisthEditor::validateLayout() {
    return console->validate();
}

void LogisthEditor::setDarkTheme(bool enabled) {
    console->setDarkTheme(enabled);
}
bool LogisthEditor::isDarkTheme() const {
    return console->isDarkTheme();
}
