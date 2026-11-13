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
    return Font(FontOptions(embeddedFace(bold)).withHeight(jmax(11.f, size * 1.18f)));
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

#include "StudioViews.h"

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
        LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label).withStandardItemHeight(28);
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
        if (bool(s.getProperties()["lane"])) {
            g.setColour((isDark ? mint : green).withAlpha(.24f));
            g.fillRoundedRectangle(b.withTrimmedTop(b.getHeight() * (1 - p)).reduced(1), 2);
        }
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

class LogisthEditor::Console : public Component,
                               private Timer,
                               public LogisthColours,
                               public FileDragAndDropTarget {
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
                    if (String(spec.id).startsWith("pat") && String(spec.id).endsWith("note"))
                        v -= 1;
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
                if (String(spec.id).endsWith("engine")) {
                    int slot = (int(id) - int(Param::o1engine)) / 16;
                    owner.sourceViews[size_t(slot)].setSelectedId(menu.getSelectedId() == 1 ? 1 : 2,
                                                                  dontSendNotification);
                }
                if (isMouseOverOrDragging(true))
                    describe();
            };
            toggle.onClick = [this] {
                if (String(spec.id) == "gmuteaudio" && toggle.getToggleState()) {
                    auto *midi = owner.processor.state.getParameter("arpout");
                    midi->beginChangeGesture();
                    midi->setValueNotifyingHost(1);
                    midi->endChangeGesture();
                }
                describe();
            };
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
            menu.setColour(ComboBox::textColourId,
                           (isDark || owner.darkTheme) ? owner.screenText : owner.ink);
            setBounds(b);
            setVisible(true);
            label.setText(title, dontSendNotification);
            label.setColour(Label::textColourId, isDark ? owner.mint.withAlpha(.7f) : owner.muted);
            slider.getProperties().set("lane", String(spec.id).startsWith("pat") ||
                                                   String(spec.id).startsWith("step"));
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
            if (kind == Style::Menu)
                menu.setSelectedId(int(owner.value(id)) + 1, dontSendNotification);
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
            return (spec.low >= 0 && spec.high <= 1 && String(spec.id) != "portamento") ||
                   String(spec.id).endsWith("gspace") || String(spec.id).endsWith("gwave");
        }
        String formatted(double v) const {
            const String name(spec.id);
            if (name.startsWith("pat") && name.endsWith("note"))
                return String(int(v) + 1);
            if (name.contains("env") &&
                (name.endsWith("enva") || name.endsWith("envd") || name.endsWith("envr")))
                return v < 1 ? String(v * 1000, 0) + "ms" : String(v, 1) + "s";
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
    class PatternCanvas : public Component {
      public:
        explicit PatternCanvas(Console &c) : owner(c) {
            setComponentID("pattern.canvas");
            setName("Draw pattern lanes");
            setWantsKeyboardFocus(true);
            addChildComponent(input);
            input.setJustification(Justification::centred);
            input.onReturnKey = [this] { commit(); };
            input.onEscapeKey = [this] { input.setVisible(false); };
            input.onFocusLost = [this] { commit(); };
        }
        ~PatternCanvas() override {
            finishGesture();
            input.onFocusLost = nullptr;
        }
        void paint(Graphics &g) override {
            int step = owner.processor.arpPlayhead.load();
            for (int row = 0; row < 7; ++row) {
                int lane = std::min(row, 5);
                if (row == 6)
                    lane = 3;
                int length = int(owner.value(Param(size_t(Param::arpnotelen) + size_t(lane))));
                for (int col = 0; col < 16; ++col)
                    if (owner.stepPage * 16 + col >= length) {
                        g.setColour(owner.screen.withAlpha(.55f));
                        g.fillRect(col * 26, row * 22, 24, 19);
                    }
                if (step >= 0) {
                    int col = step % length - owner.stepPage * 16;
                    if (col >= 0 && col < 16) {
                        g.setColour(owner.mint.withAlpha(.65f));
                        g.drawRoundedRectangle(
                            Rectangle<float>(float(col * 26), float(row * 22), 24, 19), 2, 1);
                    }
                }
            }
        }
        void mouseDown(const MouseEvent &e) override {
            if (e.mods.isRightButtonDown() || e.getNumberOfClicks() > 1) {
                showValue(e.position);
                return;
            }
            paintRow = std::clamp(int(e.position.y) / 22, 0, 6);
            drag(e.position, true);
        }
        void mouseDrag(const MouseEvent &e) override {
            if (!input.isVisible())
                drag(e.position, false);
        }
        void mouseUp(const MouseEvent &) override {
            finishGesture();
        }
        void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
            int row = std::clamp(int(e.position.y) / 22, 0, 6),
                column = std::clamp(int(e.position.x) / 26, 0, 15);
            auto *p = parameter(row, column);
            auto *spec = specification(row, column);
            float current = owner.processor.state.getRawParameterValue(spec->id)->load();
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(
                jlimit(spec->low, spec->high, current + (wheel.deltaY > 0 ? 1.f : -1.f))));
            p->endChangeGesture();
        }
        void visibilityChanged() override {
            if (!isVisible()) {
                finishGesture();
                input.setVisible(false);
            }
        }

      private:
        String identifier(int row, int column) const {
            int step = owner.stepPage * 16 + column + 1;
            if (row == 2 || row == 6)
                return "step" + String(step) + (row == 2 ? "velocity" : "hold");
            return "pat" + String(step) +
                   StringArray{"note", "oct", "", "gate", "repeat", "chance"}[row];
        }
        const ParameterSpec *specification(int row, int column) const {
            auto id = identifier(row, column);
            for (const auto &spec : parameterSpecs)
                if (id == spec.id)
                    return &spec;
            jassertfalse;
            return &parameterSpecs[0];
        }
        RangedAudioParameter *parameter(int row, int column) const {
            return owner.processor.state.getParameter(identifier(row, column));
        }
        void finishGesture() {
            if (active) {
                active->endChangeGesture();
                active = nullptr;
            }
        }
        void drag(Point<float> at, bool initial) {
            int column = std::clamp(int(at.x) / 26, 0, 15);
            auto *p = parameter(paintRow, column);
            auto *spec = specification(paintRow, column);
            if (active != p) {
                finishGesture();
                active = p;
                active->beginChangeGesture();
            }
            float proportion = 1 - jlimit(0.f, 18.f, at.y - paintRow * 22) / 18;
            float value = std::round(spec->low + (spec->high - spec->low) * proportion);
            if (paintRow == 6) {
                if (!initial && lastColumn == column)
                    return;
                value =
                    owner.processor.state.getRawParameterValue(spec->id)->load() > 0 ? 0.f : 1.f;
            }
            active->setValueNotifyingHost(active->convertTo0to1(value));
            lastColumn = column;
        }
        void showValue(Point<float> at) {
            finishGesture();
            inputRow = std::clamp(int(at.y) / 22, 0, 6);
            inputColumn = std::clamp(int(at.x) / 26, 0, 15);
            const auto *spec = specification(inputRow, inputColumn);
            float value = owner.processor.state.getRawParameterValue(spec->id)->load();
            if (inputRow == 0)
                value += 1;
            input.setBounds(inputColumn * 26, inputRow * 22, 24, 19);
            input.setText(String(int(value)), false);
            input.setColour(TextEditor::backgroundColourId, owner.screen);
            input.setColour(TextEditor::textColourId, owner.screenText);
            input.setFont(font(10));
            input.setVisible(true);
            input.grabKeyboardFocus();
            input.selectAll();
        }
        void commit() {
            if (!input.isVisible())
                return;
            auto *p = parameter(inputRow, inputColumn);
            auto *spec = specification(inputRow, inputColumn);
            float value = float(input.getText().getDoubleValue()) - (inputRow == 0 ? 1.f : 0.f);
            input.setVisible(false);
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(jlimit(spec->low, spec->high, value)));
            p->endChangeGesture();
        }
        Console &owner;
        TextEditor input;
        RangedAudioParameter *active = nullptr;
        int paintRow = 0, lastColumn = -1, inputRow = 0, inputColumn = 0;
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
        : processor(p), look(skin), keyboard(p.keyboard, *this), patternCanvas(*this),
          graphView(p, *this, [](float h) { return font(h); }),
          waveView(p, *this, [](float h) { return font(h); }),
          scopeView(p, *this, [](float h) { return font(h); }), tooltips(this, 650) {
        setLookAndFeel(&look);
        addChildComponent(patternCanvas);
        addChildComponent(graphView);
        addChildComponent(waveView);
        addAndMakeVisible(scopeView);
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
        StringArray names{"ARPEGGIATOR", "DISTORTION", "PHASER",     "CHORUS", "EQUALIZER",
                          "REVERB",      "DELAY",      "COMPRESSOR", "CHORDS", "SAMPLE"};
        for (int i = 0; i < 10; ++i) {
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
        presets.setTextWhenNothingSelected("Current patch");
        presets.getProperties().set("fontSize", 12.f);
        refreshPresets();
        presets.onChange = [this] {
            int selected = presets.getSelectedId();
            if (selected >= 1 && selected <= 10) {
                processor.loadPreset(selected - 1);
                readout = "Factory sound / " + presets.getText();
            } else if (selected >= 1000 && selected - 1000 < userPresets.size())
                processor.loadPresetAsync(userPresets[selected - 1000]);
            repaint();
        };
        addAndMakeVisible(presets);
        previous.setButtonText(String(CharPointer_UTF8("‹")));
        next.setButtonText(String(CharPointer_UTF8("›")));
        previous.onClick = [this] {
            int i = presets.getSelectedItemIndex();
            presets.setSelectedItemIndex(i <= 0 ? presets.getNumItems() - 1 : i - 1,
                                         sendNotificationSync);
        };
        next.onClick = [this] {
            int i = presets.getSelectedItemIndex();
            presets.setSelectedItemIndex(i < 0 || i >= presets.getNumItems() - 1 ? 0 : i + 1,
                                         sendNotificationSync);
        };
        presetFiles.setButtonText("PATCH");
        presetFiles.onClick = [this] { presetMenu(); };
        addAndMakeVisible(presetFiles);
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
        for (int i = 0; i < 4; ++i) {
            auto &view = sourceViews[size_t(i)];
            view.addItemList({"VOICE", "SOURCE", "ENV", "GRAINS", "MOTION", "TIME"}, 1);
            view.setSelectedId(
                processor.state.getRawParameterValue("o" + String(i + 1) + "engine")->load() > 0
                    ? 2
                    : 1);
            view.setName("Oscillator view");
            view.onChange = [this] {
                layout();
                repaint();
            };
            addChildComponent(view);
            auto &load = sourceLoad[size_t(i)];
            load.setButtonText("IMPORT");
            load.onClick = [this, i] { chooseSource(i); };
            addChildComponent(load);
            auto &clear = sourceClear[size_t(i)];
            clear.setButtonText("RESET");
            clear.onClick = [this, i] {
                processor.clearSource(i);
                repaint();
            };
            addChildComponent(clear);
            auto &frame = sourceFrame[size_t(i)];
            frame.addItemList({"256", "512", "1024", "2048"}, 1);
            frame.setSelectedId(4);
            frame.setTooltip("Samples per wavetable frame on import");
            addChildComponent(frame);
        }
        for (int i = 0; i < 4; ++i) {
            sourceWave[size_t(i)].onClick = [this, i] {
                waveView.setSlot(i);
                waveSlot.setSelectedId(i + 1, dontSendNotification);
                setRack(9);
            };
            sourceWave[size_t(i)].setTooltip("Open sample waveform and playback position");
            addChildComponent(sourceWave[size_t(i)]);
        }
        StringArray graphNames{"NOTE",      "RAND SEL", "OCTAVE", "PITCH",
                               "VEL SCALE", "GATE",     "LATE",   "HARMONY"};
        for (int i = 0; i < 8; ++i) {
            auto &tab = graphTabs[size_t(i)];
            tab.setButtonText(graphNames[i]);
            tab.getProperties().set("dark", true);
            tab.onClick = [this, i] {
                graphView.setLane(i);
                layout();
                repaint();
            };
            addChildComponent(tab);
        }
        graphCopy.setButtonText("COPY");
        graphCopy.onClick = [this] { processor.copyGraph(graphView.selectedLane()); };
        addChildComponent(graphCopy);
        graphPaste.setButtonText("PASTE");
        graphPaste.onClick = [this] {
            processor.pasteGraph(graphView.selectedLane());
            layout();
            repaint();
        };
        addChildComponent(graphPaste);
        graphDice.setButtonText("RAND");
        graphDice.onClick = [this] {
            processor.randomiseGraph(graphView.selectedLane());
            repaint();
        };
        addChildComponent(graphDice);
        graphBankCopy.setButtonText("COPY TO");
        graphBankCopy.onClick = [this] {
            PopupMenu menu;
            for (int i = 0; i < 12; ++i)
                menu.addItem(i + 1, String::charToString(juce_wchar('A' + i)));
            Component::SafePointer<Console> safe(this);
            int from = patternSlot.getSelectedId() - 1;
            menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(&graphBankCopy),
                               [safe, from](int to) {
                                   if (safe && to > 0)
                                       safe->processor.copyPattern(from, to - 1);
                               });
        };
        addChildComponent(graphBankCopy);
        patternTitle.setEditable(false, true, false);
        patternTitle.setColour(Label::backgroundColourId, Colour(0xff30443b));
        patternTitle.setFont(font(11));
        patternTitle.onTextChange = [this] {
            processor.setPatternName(patternSlot.getSelectedId() - 1, patternTitle.getText());
        };
        addChildComponent(patternTitle);
        waveSlot.addItemList({"A1", "A2", "B1", "B2"}, 1);
        waveSlot.setSelectedId(1);
        waveSlot.onChange = [this] { waveView.setSlot(waveSlot.getSelectedId() - 1); };
        waveSlot.getProperties().set("dark", true);
        addChildComponent(waveSlot);
        for (int i = 0; i < 3; ++i) {
            auto &button = scopeSources[size_t(i)];
            button.setButtonText(i == 0 ? "A" : i == 1 ? "B" : "L+R");
            button.onClick = [this, i] {
                scopeView.setSource(i);
                processor.state.state.setProperty("uiScope", i, nullptr);
                for (int j = 0; j < 3; ++j)
                    scopeSources[size_t(j)].setToggleState(j == i, dontSendNotification);
            };
            addAndMakeVisible(button);
        }
        int initialScope = jlimit(0, 2, int(processor.state.state.getProperty("uiScope", 2)));
        scopeView.setSource(initialScope);
        scopeSources[size_t(initialScope)].setToggleState(true, dontSendNotification);
        patternSlot.addItemList({"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L"}, 1);
        patternSlot.setSelectedId(1);
        patternSlot.setTooltip("Recall pattern slot; SAVE stores the current pattern");
        patternSlot.onChange = [this] {
            processor.recallPattern(patternSlot.getSelectedId() - 1);
            layout();
            repaint();
        };
        addChildComponent(patternSlot);
        patternSave.setButtonText("SAVE");
        patternSave.onClick = [this] {
            processor.storePattern(patternSlot.getSelectedId() - 1);
            readout = "Pattern stored";
            repaint();
        };
        addChildComponent(patternSave);
        patternRandom.setButtonText("DICE");
        patternRandom.onClick = [this] {
            processor.randomisePattern();
            repaint();
        };
        addChildComponent(patternRandom);
        for (int note = 0; note < 128; ++note)
            chordTrigger.addItem(
                MidiMessage::getMidiNoteName(note, true, true, 4) + " / " + String(note), note + 1);
        chordTrigger.setSelectedId(61);
        chordTrigger.setName("Chord trigger note");
        chordTrigger.onChange = [this] {
            refreshChord();
            repaint();
        };
        addChildComponent(chordTrigger);
        chordType.addItemList({"Unison", "Major", "Minor", "Maj 7", "Min 7", "Dominant 7", "Sus 2",
                               "Sus 4", "Power", "Diminished", "Augmented"},
                              1);
        chordType.setTextWhenNothingSelected("Voicing");
        chordType.onChange = [this] {
            processor.setChord(chordTrigger.getSelectedId() - 1, chordType.getSelectedId() - 1);
            refreshChord();
        };
        addChildComponent(chordType);
        chordLearn.setButtonText("LEARN");
        chordLearn.setTooltip("Collect up to eight MIDI notes, then click DONE");
        chordLearn.onClick = [this] {
            if (processor.learning() >= 0)
                processor.finishLearn();
            else
                processor.learnChord(chordTrigger.getSelectedId() - 1);
            refreshChord();
        };
        addChildComponent(chordLearn);
        chordCopy.setButtonText("MAP ALL");
        chordCopy.onClick = [this] {
            int type = chordType.getSelectedId() - 1;
            if (type >= 0)
                for (int key = 0; key < 128; ++key)
                    processor.setChord(key, type);
            refreshChord();
        };
        chordCopy.setTooltip("Map this voicing type to every trigger, transposed to its root");
        addChildComponent(chordCopy);
        for (int i = 0; i < 8; ++i) {
            auto &note = chordNotes[size_t(i)];
            note.setSliderStyle(Slider::LinearBar);
            note.getProperties().set("number", true);
            note.setTextBoxStyle(Slider::TextBoxLeft, false, 50, 28);
            note.setRange(-1, 127, 1);
            note.textFromValueFunction = [](double value) {
                return value < 0 ? String("-")
                                 : MidiMessage::getMidiNoteName(int(value), true, true, 4);
            };
            note.valueFromTextFunction = [](const String &text) { return text.getDoubleValue(); };
            note.onValueChange = [this, i] {
                if (!updatingChord) {
                    processor.setChordNote(chordTrigger.getSelectedId() - 1, i,
                                           int(chordNotes[size_t(i)].getValue()));
                    refreshChord();
                }
            };
            note.setTooltip("MIDI note: drag or enter 0–127; -1 removes a voice");
            addChildComponent(note);
        }
        for (Component *widget :
             std::array<Component *, 7>{&patternSlot, &patternSave, &patternRandom, &chordTrigger,
                                        &chordType, &chordLearn, &chordCopy})
            widget->getProperties().set("dark", true);
        for (auto &note : chordNotes)
            note.getProperties().set("dark", true);
        refreshChord();
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
    bool isInterestedInFileDrag(const StringArray &files) override {
        return files.size() == 1 && File(files[0]).hasFileExtension("wav;aif;aiff;flac");
    }
    void filesDropped(const StringArray &files, int x, int y) override {
        if (!isInterestedInFileDrag(files))
            return;
        int slot = y >= 82 && y <= 266 ? (x >= 16 && x <= 476     ? selectedPart * 2
                                          : x >= 764 && x <= 1224 ? selectedPart * 2 + 1
                                                                  : -1)
                                       : -1;
        if (slot < 0) {
            readout = "Drop audio on an oscillator panel";
            repaint();
            return;
        }
        auto *engine = processor.state.getParameter("o" + String(slot + 1) + "engine");
        if (processor.state.getRawParameterValue("o" + String(slot + 1) + "engine")->load() == 0) {
            engine->beginChangeGesture();
            engine->setValueNotifyingHost(engine->convertTo0to1(2));
            engine->endChangeGesture();
        }
        sourceViews[size_t(slot)].setSelectedId(2, dontSendNotification);
        processor.loadSourceAsync(slot, File(files[0]),
                                  256 << (sourceFrame[size_t(slot)].getSelectedId() - 1));
        layout();
        repaint();
    }
    void setDarkTheme(bool enabled, bool persist = true) {
        choosePalette(enabled);
        look.setDarkTheme(enabled);
        theme.setToggleState(enabled, dontSendNotification);
        theme.setButtonText(enabled ? "DARK" : "LIGHT");
        keyboard.applyTheme();
        patternTitle.setColour(Label::textColourId, screenText);
        waveSlot.setColour(ComboBox::textColourId, screenText);
        for (auto *button : {&graphCopy, &graphPaste, &graphDice, &graphBankCopy}) {
            button->setColour(TextButton::buttonColourId, Colour(0xff30443b));
            button->setColour(TextButton::textColourOffId, screenText);
        }
        for (ComboBox *box : {&patternSlot, &chordTrigger, &chordType})
            box->setColour(ComboBox::textColourId, screenText);
        for (TextButton *button : {&patternSave, &patternRandom, &chordLearn, &chordCopy}) {
            button->setColour(TextButton::buttonColourId, Colour(0xff30443b));
            button->setColour(TextButton::textColourOffId, screenText);
        }
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
        rack = jlimit(0, 9, page);
        layout();
        repaint();
    }
    void showScenario(int index) {
        if (index >= 8) {
            selectedPart = 0;
            rack = index == 8 ? 9 : 0;
            previewPattern = 1;
            previewGraphs = 1;
            previewGran = 1;
            previewSource = 3;
            graphView.setLane(index == 9 ? 3 : 0);
            for (auto &v : sourceViews)
                v.setSelectedId(index == 10   ? 4
                                : index == 11 ? 5
                                : index == 12 ? 6
                                              : 2,
                                dontSendNotification);
            layout();
            repaint();
            return;
        }
        previewGraphs = 0;
        if (index >= 4) {
            selectedPart = 0;
            rack = index == 6 ? 8 : 0;
            stepPage = index == 5 ? 1 : 0;
            previewPattern = 1;
            previewSource = index == 4 ? 1 : 3;
            for (auto &v : sourceViews)
                v.setSelectedId(2, dontSendNotification);
            layout();
            repaint();
            return;
        }
        previewPattern = previewSource = -1;
        for (auto &v : sourceViews)
            v.setSelectedId(1, dontSendNotification);
        selectedPart = index == 1 ? 1 : 0;
        rack = index == 1 ? 6 : index == 2 ? 2 : 0;
        stepPage = index == 3 ? 1 : 0;
        layout();
        repaint();
    }
    String validate() {
        const int oldPart = selectedPart, oldRack = rack, oldStep = stepPage,
                  oldGraph = graphView.selectedLane();
        std::array<int, 4> views;
        for (int i = 0; i < 4; ++i)
            views[size_t(i)] = sourceViews[size_t(i)].getSelectedId();
        std::set<size_t> visited;
        String error;
        auto inspect = [&] {
            layout();
            if (graphView.isVisible())
                for (size_t i = 0; i < parameterCount; ++i)
                    if (logisth_ui::GraphView::ownsParameter(parameterSpecs[i].id)) {
                        if (!processor.state.getParameter(parameterSpecs[i].id))
                            error = "Missing graph binding";
                        visited.insert(i);
                    }
            for (size_t i = 0; i < controls.size(); ++i)
                if (controls[i]->isVisible()) {
                    visited.insert(i);
                    auto *hit = getComponentAt(controls[i]->getBounds().getCentre());
                    if (hit != controls[i].get() && !controls[i]->isParentOf(hit) &&
                        hit != &patternCanvas)
                        error = "Obscured: " + String(parameterSpecs[i].id);
                    if (!getLocalBounds().contains(controls[i]->getBounds()) ||
                        controls[i]->getWidth() < 20 || controls[i]->getHeight() < 18)
                        error = "Invalid bounds: " + String(parameterSpecs[i].id);
                    for (size_t j = 0; j < i; ++j)
                        if (controls[j]->isVisible() &&
                            controls[i]->getBounds().intersects(controls[j]->getBounds()))
                            error = "Overlap: " + String(parameterSpecs[i].id) + " / " +
                                    parameterSpecs[j].id;
                }
            for (auto *component : getChildren())
                if (component->isVisible() && component != &keyboard &&
                    dynamic_cast<Control *>(component) == nullptr) {
                    if (!getLocalBounds().contains(component->getBounds()))
                        error = "Widget outside console: " + component->getName();
                }
        };
        for (auto &v : sourceViews)
            v.setSelectedId(1, dontSendNotification);
        previewPattern = 0;
        previewGraphs = 0;
        previewSource = 0;
        for (int clock = 0; clock < 2; ++clock) {
            previewSync = clock;
            for (int part = 0; part < 2; ++part)
                for (int page = 0; page < 10; ++page)
                    for (int step = 0; step < 2; ++step) {
                        selectedPart = part;
                        rack = page;
                        stepPage = step;
                        inspect();
                    }
        }
        rack = 0;
        previewPattern = 1;
        for (int step = 0; step < 2; ++step) {
            stepPage = step;
            inspect();
        }
        previewPattern = 1;
        previewGraphs = 1;
        previewLink = 0;
        for (int graph = 0; graph < 8; ++graph)
            for (int clock = 0; clock < 2; ++clock) {
                graphView.setLane(graph);
                previewSync = clock;
                inspect();
            }
        previewPattern = 0;
        previewGraphs = 0;
        previewGran = 0;
        for (int part = 0; part < 2; ++part)
            for (int mode = 0; mode < 4; ++mode)
                for (int view = 1; view <= 6; ++view) {
                    selectedPart = part;
                    previewSource = mode;
                    for (auto &v : sourceViews)
                        v.setSelectedId(view, dontSendNotification);
                    inspect();
                }
        for (size_t i = 0; i < controls.size(); ++i)
            if (!visited.count(i))
                error = "Unreachable: " + String(parameterSpecs[i].id);
        previewGran = 1;
        for (int part = 0; part < 2; ++part) {
            selectedPart = part;
            for (auto &v : sourceViews)
                v.setSelectedId(2, dontSendNotification);
            inspect();
        }
        selectedPart = oldPart;
        rack = oldRack;
        stepPage = oldStep;
        previewSync = previewSource = previewPattern = previewGraphs = previewGran = previewLink =
            -1;
        graphView.setLane(oldGraph);
        for (int i = 0; i < 4; ++i)
            sourceViews[size_t(i)].setSelectedId(views[size_t(i)], dontSendNotification);
        layout();
        return error;
    }

  private:
    friend class Control;
    void shell(Graphics &g, Rectangle<int> b, const String &title, const String &hint = {}) {
        paintShell(g, b, title, hint, *this);
    }
    static constexpr std::array<Param, 9> enables{Param::arpon,    Param::diston, Param::phaseron,
                                                  Param::choruson, Param::eqon,   Param::reverbon,
                                                  Param::delayon,  Param::compon, Param::chordon};
    float value(Param p) const {
        if (previewGraphs >= 0 && p == Param::arpgraphs)
            return float(previewGraphs);
        if (previewGran >= 0 && String(parameterSpecs[size_t(p)].id).endsWith("gmodel"))
            return float(previewGran);
        if (previewLink >= 0 && p == Param::glink)
            return float(previewLink);
        if (previewSource >= 0 && String(parameterSpecs[size_t(p)].id).endsWith("engine"))
            return float(previewSource);
        if (previewPattern >= 0 && p == Param::arppattern)
            return float(previewPattern);
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
    void refreshPresets() {
        int previousId = presets.getSelectedId();
        presets.clear(dontSendNotification);
        presets.addSectionHeading("FACTORY");
        presets.addItemList({"Init / analog", "Wide supersaw", "Sub bass", "Slow atmosphere",
                             "Pulse pluck", "Step sequence", "Morph glass", "Sample keys",
                             "Grain cloud", "Chord pulse"},
                            1);
        userPresets =
            processor.presetDirectory().findChildFiles(File::findFiles, false, "*.logisth");
        userPresets.sort();
        auto active = processor.currentPresetFile();
        if (active.existsAsFile() && !userPresets.contains(active))
            userPresets.add(active);
        if (!userPresets.isEmpty())
            presets.addSectionHeading("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presets.addItem(userPresets[i].getFileNameWithoutExtension(), 1000 + i);
        int current = userPresets.indexOf(active);
        if (current >= 0)
            presets.setSelectedId(1000 + current, dontSendNotification);
        else if (processor.currentPresetName().isNotEmpty())
            presets.setText(processor.currentPresetName(), dontSendNotification);
        else if (previousId > 0 && previousId <= 10)
            presets.setSelectedId(previousId, dontSendNotification);
        else
            presets.setSelectedId(0, dontSendNotification);
        presetRevision = processor.presetRevision.load();
    }
    void presetMenu() {
        PopupMenu menu;
        menu.addItem(1, "Save as...");
        auto current = processor.currentPresetFile();
        menu.addItem(2, "Overwrite current user preset", current != File{});
        menu.addItem(3, "Load preset file...");
        menu.addSeparator();
        menu.addItem(4, "Choose library folder...");
        menu.addItem(5, "Refresh library");
        menu.addItem(6, "Move current preset to trash", current.existsAsFile());
        Component::SafePointer<Console> safe(this);
        menu.showMenuAsync(
            PopupMenu::Options{}.withTargetComponent(&presetFiles), [safe, current](int choice) {
                if (!safe)
                    return;
                if (choice == 2) {
                    safe->processor.savePresetAsync(current);
                    return;
                }
                if (choice == 5) {
                    safe->refreshPresets();
                    return;
                }
                if (choice == 6) {
                    AlertWindow::showOkCancelBox(
                        AlertWindow::QuestionIcon, "Delete user preset?",
                        "Move " + current.getFileName() + " to the trash?", "Move to trash",
                        "Cancel", safe.getComponent(),
                        ModalCallbackFunction::create([safe, current](int confirmed) {
                            if (safe && confirmed && current.moveToTrash()) {
                                safe->readout = "Preset moved to trash";
                                safe->refreshPresets();
                                safe->repaint();
                            }
                        }));
                    return;
                }
                if (choice != 1 && choice != 3 && choice != 4)
                    return;
                auto directory = safe->processor.presetDirectory();
                safe->presetChooser = std::make_unique<FileChooser>(
                    choice == 1   ? "Save logisth preset"
                    : choice == 3 ? "Load logisth preset"
                                  : "Choose preset library folder",
                    choice == 1 ? directory.getChildFile("New preset.logisth") : directory,
                    choice == 4 ? String{} : "*.logisth");
                int flags =
                    choice == 4 ? FileBrowserComponent::openMode |
                                      FileBrowserComponent::canSelectDirectories
                    : choice == 1
                        ? FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
                              FileBrowserComponent::warnAboutOverwriting
                        : FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
                safe->presetChooser->launchAsync(flags, [safe, choice](const FileChooser &chooser) {
                    if (!safe)
                        return;
                    auto file = chooser.getResult();
                    if (file == File{})
                        return;
                    if (choice == 4) {
                        safe->processor.setPresetDirectory(file);
                        safe->refreshPresets();
                    } else if (choice == 1)
                        safe->processor.savePresetAsync(file.withFileExtension(".logisth"));
                    else
                        safe->processor.loadPresetAsync(file);
                });
            });
    }
    void graphLayout() {
        put(Param::arppattern, {414, 289, 86, 23}, Style::Menu, {}, true);
        put(Param::arpgraphs, {506, 289, 65, 23}, Style::Toggle, "GRAPHS", true);
        patternSlot.setBounds(577, 289, 48, 23);
        patternSlot.setVisible(true);
        patternTitle.setBounds(631, 289, 75, 23);
        put(Param::arplatch, {712, 289, 53, 23}, Style::Toggle, "LATCH", true);
        patternTitle.setVisible(true);
        if (!patternTitle.isBeingEdited())
            patternTitle.setText(processor.patternName(patternSlot.getSelectedId() - 1),
                                 dontSendNotification);
        put(Param::gretrigger, {771, 289, 48, 23}, Style::Toggle, "RETR", true);
        put(Param::glink, {825, 289, 48, 23}, Style::Toggle, "LINK", true);
        put(Param::gchord, {879, 289, 52, 23}, Style::Toggle, "CHORD", true);
        for (int i = 0; i < 8; ++i) {
            graphTabs[size_t(i)].setBounds(414 + i * 65, 319, 62, 22);
            graphTabs[size_t(i)].setVisible(true);
            graphTabs[size_t(i)].setToggleState(i == graphView.selectedLane(),
                                                dontSendNotification);
        }
        int graph = graphView.selectedLane(), lengthLane = value(Param::glink) > 0 ? 0 : graph;
        put(Param(size_t(Param::glen0) + size_t(lengthLane) * 2), {449, 347, 39, 22}, Style::Number,
            {}, true);
        put(Param(size_t(Param::gdiv0) + size_t(graph) * 2), {532, 347, 37, 22}, Style::Number, {},
            true);
        put(value(Param::sync) > 0 ? Param::gdivision : Param::gfree, {617, 347, 72, 22},
            value(Param::sync) > 0 ? Style::Menu : Style::Number, {}, true);
        graphCopy.setBounds(699, 347, 50, 22);
        graphCopy.setVisible(true);
        graphPaste.setBounds(756, 347, 50, 22);
        graphPaste.setVisible(true);
        graphDice.setBounds(813, 347, 50, 22);
        graphDice.setVisible(true);
        graphBankCopy.setBounds(870, 347, 60, 22);
        graphBankCopy.setVisible(true);
        graphView.setBounds(414, 372, 520, 160);
        graphView.setVisible(true);
        graphView.toFront(false);
    }

    void chooseSource(int index) {
        sourceChooser = std::make_unique<FileChooser>("Import audio source", File{},
                                                      "*.wav;*.aif;*.aiff;*.flac");
        int frame = 256 << (sourceFrame[size_t(index)].getSelectedId() - 1);
        Component::SafePointer<Console> safe(this);
        sourceChooser->launchAsync(FileBrowserComponent::openMode |
                                       FileBrowserComponent::canSelectFiles,
                                   [safe, index, frame](const FileChooser &chooser) {
                                       if (safe) {
                                           auto file = chooser.getResult();
                                           if (file.existsAsFile())
                                               safe->processor.loadSourceAsync(index, file, frame);
                                       }
                                   });
    }
    void refreshChord() {
        updatingChord = true;
        const auto map = processor.chordMap();
        const auto &chord = map->keys[size_t(chordTrigger.getSelectedId() - 1)];
        for (int i = 0; i < 8; ++i)
            chordNotes[size_t(i)].setValue(i < chord.count ? chord.notes[size_t(i)] : -1,
                                           dontSendNotification);
        chordLearn.setButtonText(processor.learning() >= 0 ? "DONE" : "LEARN");
        updatingChord = false;
    }
    void oscillator(int number, Rectangle<int> b) {
        String id = "o" + String(number);
        const size_t slot = size_t(number - 1);
        control(id + "engine", {b.getX() + 300, b.getY() + 6, 146, 24}, Style::Menu);
        sourceViews[slot].setBounds(b.getX() + 191, b.getY() + 6, 99, 24);
        sourceViews[slot].setVisible(true);
        int view = sourceViews[slot].getSelectedId();
        sourceWave[slot].setBounds(b.getX() + 86, b.getY() + 118, 358, 10);
        sourceWave[slot].setVisible(view == 2);
        if (view == 1) {
            StringArray names{"VOLUME", "PHASE", "DETUNE", "STEREO", "PAN"};
            StringArray suffix{"level", "phase", "detune", "stereo", "pan"};
            for (int i = 0; i < 3; ++i)
                control(id + StringArray{"octave", "tune", "fine"}[i],
                        {b.getX() + 12, b.getY() + 34 + i * 39, 62, 33}, Style::Number,
                        StringArray{"OCTAVE", "NOTE", "FINE"}[i]);
            for (int i = 0; i < 5; ++i)
                control(id + suffix[i], {b.getX() + 86 + i * 72, b.getY() + 35, 66, 86},
                        Style::Knob, names[i]);
            control(id + "wave", {b.getX() + 87, b.getY() + 137, 120, 26}, Style::Menu);
            control(id + "voices", {b.getX() + 217, b.getY() + 130, 58, 38}, Style::Number,
                    "VOICES");
            control(id + "retrig", {b.getX() + 287, b.getY() + 141, 74, 22}, Style::Toggle,
                    "RETRIG");
            control(id + "invert", {b.getX() + 371, b.getY() + 141, 74, 22}, Style::Toggle,
                    "INVERT");
            controls[size_t(Param(size_t(Param::o1wave) + size_t(number - 1) * 12))]->enable(
                value(Param(size_t(Param::o1engine) + slot * 16)) == 0);
            controls[size_t(Param(size_t(Param::o1retrig) + size_t(number - 1) * 12))]->enable(
                value(Param(size_t(Param::o1engine) + slot * 16)) <= 1);
            controls[size_t(Param(size_t(Param::o1phase) + size_t(number - 1) * 12))]->enable(
                value(Param(size_t(Param::o1retrig) + size_t(number - 1) * 12)) > 0 &&
                value(Param(size_t(Param::o1engine) + slot * 16)) <= 1);
        } else {
            for (int i = 0; i < 3; ++i)
                control(id + StringArray{"octave", "tune", "fine"}[i],
                        {b.getX() + 12, b.getY() + 34 + i * 39, 62, 33}, Style::Number,
                        StringArray{"OCTAVE", "NOTE", "FINE"}[i]);
            if (view == 4) {
                control(id + "gmodel", {b.getX() + 88, b.getY() + 35, 150, 24}, Style::Menu);
                for (int i = 0; i < 4; ++i)
                    control(id + "g" + StringArray{"attack", "hold", "space", "wave"}[i],
                            {b.getX() + 87 + i * 90, b.getY() + 68, 76, 96}, Style::Knob,
                            StringArray{"ATTACK", "HOLD", "GR.SPACE", "WAVE.SPACE"}[i]);
            } else if (view == 5) {
                for (int i = 0; i < 4; ++i)
                    control(id + "g" + StringArray{"pan", "depth", "speed", "random"}[i],
                            {b.getX() + 87 + i * 90, b.getY() + 38, 76, 88}, Style::Knob,
                            StringArray{"PAN", "DEPTH", "SPEED", "RAND"}[i]);
                control(id + "gfreeze", {b.getX() + 88, b.getY() + 141, 95, 22}, Style::Toggle,
                        "TIME HOLD");
                control(id + "loop", {b.getX() + 196, b.getY() + 141, 85, 22}, Style::Toggle,
                        "LOOP");
                control(id + "reverse", {b.getX() + 294, b.getY() + 141, 104, 22}, Style::Toggle,
                        "REVERSE");
            } else if (view == 6) {
                control(id + "gtrans", {b.getX() + 88, b.getY() + 36, 105, 24}, Style::Menu);
                control(id + "gkey", {b.getX() + 203, b.getY() + 36, 115, 24}, Style::Menu);
                control(id + "gsync", {b.getX() + 328, b.getY() + 36, 116, 24}, Style::Toggle,
                        "TEMPO SYNC");
                control(id + "gthold", {b.getX() + 88, b.getY() + 73, 80, 91}, Style::Knob,
                        "TRANS HOLD");
                control(id + "gbeats", {b.getX() + 188, b.getY() + 73, 80, 91}, Style::Knob,
                        "SAMPLE BEATS");
            } else if (view == 3) {
                control(id + "envelope", {b.getX() + 88, b.getY() + 36, 150, 22}, Style::Toggle,
                        "OSC ENVELOPE");
                for (int i = 0; i < 4; ++i)
                    control(id + "env" + StringArray{"a", "d", "s", "r"}[i],
                            {b.getX() + 90 + i * 89, b.getY() + 65, 70, 99}, Style::Fader,
                            StringArray{"A", "D", "S", "R"}[i]);
            } else {
                int mode = int(value(Param(size_t(Param::o1engine) + slot * 16)));
                StringArray names =
                    mode == 3   ? StringArray{"VOLUME", "POSITION", "SIZE", "DENSITY", "SPRAY"}
                    : mode == 2 ? StringArray{"VOLUME", "START", "END", "STEREO", "PAN"}
                                : StringArray{"VOLUME", "POSITION", "WARP", "STEREO", "PAN"};
                StringArray suffix =
                    mode == 3   ? StringArray{"level", "position", "size", "density", "spray"}
                    : mode == 2 ? StringArray{"level", "position", "end", "stereo", "pan"}
                                : StringArray{"level", "position", "warp", "stereo", "pan"};
                if (mode == 3 && value(Param(size_t(Param::o1gmodel) + slot * 15)) > 0) {
                    names = {"VOLUME", "START", "HOLD", "GR.SPACE", "WAVE.SPACE"};
                    suffix = {"level", "position", "ghold", "gspace", "gwave"};
                }
                for (int i = 0; i < 5; ++i)
                    control(id + suffix[i], {b.getX() + 86 + i * 72, b.getY() + 35, 66, 80},
                            Style::Knob, names[i]);
                if (mode >= 2) {
                    control(id + "root", {b.getX() + 88, b.getY() + 129, 48, 35}, Style::Number,
                            "ROOT");
                    if (mode == 3 && value(Param(size_t(Param::o1gmodel) + slot * 15)) == 0)
                        control(id + "scan", {b.getX() + 145, b.getY() + 129, 68, 35},
                                Style::Number, "SCAN");
                    else
                        control(id + "loop", {b.getX() + 145, b.getY() + 141, 68, 22},
                                Style::Toggle, "LOOP");
                    control(id + "reverse", {b.getX() + 223, b.getY() + 141, 68, 22}, Style::Toggle,
                            "REVERSE");
                    sourceLoad[slot].setBounds(b.getX() + 300, b.getY() + 141, 68, 22);
                    sourceClear[slot].setBounds(b.getX() + 379, b.getY() + 141, 65, 22);
                } else {
                    sourceLoad[slot].setBounds(b.getX() + 88, b.getY() + 141, 68, 22);
                    sourceFrame[slot].setBounds(b.getX() + 168, b.getY() + 141, 76, 22);
                    sourceFrame[slot].setVisible(true);
                    sourceClear[slot].setBounds(b.getX() + 254, b.getY() + 141, 68, 22);
                }
                sourceLoad[slot].setVisible(true);
                sourceClear[slot].setVisible(true);
            }
        }
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
        for (int i = 0; i < 10; ++i) {
            rackButtons[size_t(i)]->setBounds(291, 311 + i * 22, 108, 21);
            if (i < 9)
                put(enables[size_t(i)], {294, 312 + i * 22, 20, 20}, Style::Toggle, {}, true);
            if (i < 9)
                controls[size_t(enables[size_t(i)])]->toFront(false);
        }
        if (rack == 9) {
            waveSlot.setBounds(414, 289, 110, 23);
            waveSlot.setVisible(true);
            waveView.setBounds(414, 320, 520, 203);
            waveView.setVisible(true);
            return;
        }
        if (rack == 0 && value(Param::arppattern) > 0 && value(Param::arpgraphs) > 0) {
            graphLayout();
            return;
        }
        if (rack == 0)
            put(Param::arpgraphs, {716, 289, 88, 22}, Style::Toggle, "GRAPHS", true);
        if (rack == 8) {
            chordTrigger.setBounds(414, 329, 140, 30);
            chordTrigger.setVisible(true);
            chordType.setBounds(566, 329, 145, 30);
            chordType.setVisible(true);
            chordLearn.setBounds(724, 329, 92, 30);
            chordLearn.setVisible(true);
            chordCopy.setBounds(829, 329, 98, 30);
            chordCopy.setVisible(true);
            for (int i = 0; i < 8; ++i) {
                chordNotes[size_t(i)].setBounds(414 + i * 65, 405, 58, 36);
                chordNotes[size_t(i)].setVisible(true);
            }
            return;
        }
        if (rack == 0 && value(Param::arppattern) > 0) {
            put(Param::arppattern, {414, 320, 100, 24}, Style::Menu, {}, true);
            put(Param::arpswing, {652, 289, 60, 22}, Style::Number, {}, true);
            put(Param::arpgraphs, {720, 289, 78, 22}, Style::Toggle, "GRAPHS", true);
            put(Param::arplatch, {521, 320, 55, 24}, Style::Toggle, "LATCH", true);
            put(value(Param::sync) > 0 ? Param::arpdivision : Param::arptime, {582, 320, 64, 24},
                value(Param::sync) > 0 ? Style::Menu : Style::Number, {}, true);
            put(Param::arpvelocity, {652, 320, 100, 24}, Style::Menu, {}, true);
            patternSlot.setBounds(758, 320, 44, 24);
            patternSlot.setVisible(true);
            patternSave.setBounds(808, 320, 48, 24);
            patternSave.setVisible(true);
            patternRandom.setBounds(862, 320, 64, 24);
            patternRandom.setVisible(true);
            for (int i = 0; i < 2; ++i) {
                stepPages[size_t(i)].setButtonText(i ? "17-32" : "01-16");
                stepPages[size_t(i)].setBounds(813 + i * 60, 289, 54, 22);
                stepPages[size_t(i)].setToggleState(stepPage == i, dontSendNotification);
                stepPages[size_t(i)].setVisible(true);
            }
            for (int row = 0; row < 7; ++row) {
                if (row < 6)
                    put(Param(size_t(Param::arpnotelen) + size_t(row)),
                        {468, 359 + row * 22, 32, 19}, Style::Number, {}, true);
                for (int i = 0; i < 16; ++i) {
                    int step = stepPage * 16 + i + 1;
                    String id = row == 2   ? "step" + String(step) + "velocity"
                                : row == 6 ? "step" + String(step) + "hold"
                                           : "pat" + String(step) +
                                                 StringArray{"note",   "oct",    "", "gate",
                                                             "repeat", "chance", ""}[row];
                    control(id, {509 + i * 26, 359 + row * 22, 24, 19},
                            row == 6 ? Style::Toggle : Style::Number, {}, true);
                }
            }
            patternCanvas.setBounds(509, 359, 416, 151);
            patternCanvas.setVisible(true);
            patternCanvas.toFront(false);
            return;
        }
        if (rack == 0) {
            put(Param::arppattern, {813, 289, 112, 22}, Style::Menu, {}, true);
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
                stepPages[size_t(i)].setButtonText(i ? "09-16" : "01-08");
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
        patternCanvas.setVisible(false);
        graphView.setVisible(false);
        waveView.setVisible(false);
        waveSlot.setVisible(false);
        patternTitle.setVisible(false);
        for (auto &tab : graphTabs)
            tab.setVisible(false);
        for (auto *button : {&graphCopy, &graphPaste, &graphDice, &graphBankCopy})
            button->setVisible(false);
        for (auto &area : sourceWave)
            area.setVisible(false);
        for (auto &button : stepPages)
            button.setVisible(false);
        for (auto &w : sourceViews)
            w.setVisible(false);
        for (auto &w : sourceLoad)
            w.setVisible(false);
        for (auto &w : sourceClear)
            w.setVisible(false);
        for (auto &w : sourceFrame)
            w.setVisible(false);
        for (Component *w :
             std::array<Component *, 7>{&patternSlot, &patternSave, &patternRandom, &chordTrigger,
                                        &chordType, &chordLearn, &chordCopy})
            w->setVisible(false);
        for (auto &w : chordNotes)
            w.setVisible(false);
        presets.setBounds(364, 25, 306, 32);
        previous.setBounds(332, 25, 26, 32);
        next.setBounds(676, 25, 26, 32);
        presetFiles.setBounds(708, 25, 70, 32);
        for (int i = 0; i < 2; ++i) {
            parts[size_t(i)].setBounds(834 + i * 40, 28, 34, 24);
            parts[size_t(i)].setToggleState(i == selectedPart, dontSendNotification);
        }
        put(Param::solo, {927, 28, 68, 24}, Style::Menu);
        put(Param::polyphony, {1010, 16, 60, 38}, Style::Number, "POLY");
        put(Param::sync, {1085, 28, 64, 24}, Style::Toggle, "SYNC");
        theme.setBounds(1162, 28, 62, 24);
        put(Param::arpout, {225, 29, 82, 24}, Style::Toggle, "MIDI OUT");
        put(Param::gmuteaudio, {225, 55, 82, 19}, Style::Toggle, "MIDI ONLY");
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
        scopeView.setBounds(972, 309, 236, 85);
        for (int i = 0; i < 3; ++i)
            scopeSources[size_t(i)].setBounds(976 + i * 77, 402, 70, 22);
        put(Param::mixA, {975, 429, 45, 95}, Style::Fader, "MIX A");
        put(Param::mixB, {1031, 429, 45, 95}, Style::Fader, "MIX B");
        put(Param::master, {1087, 429, 45, 95}, Style::Fader, "MAIN");
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
        layoutStamp = currentLayoutState();
    }
    std::array<int, 15> currentLayoutState() const {
        std::array<int, 15> state{int(value(Param::sync)),       int(value(Param::lfo1free)),
                                  int(value(Param::lfo2free)),   int(value(Param::arpmode) >= 8),
                                  int(value(Param::arppattern)), int(value(Param::arpgraphs)),
                                  int(value(Param::glink))};
        for (int o = 0; o < 4; ++o) {
            state[size_t(7 + o)] = int(value(Param(size_t(Param::o1engine) + size_t(o) * 16)));
            state[size_t(11 + o)] = int(value(Param(size_t(Param::o1gmodel) + size_t(o) * 15)));
        }
        return state;
    }
    void timerCallback() override {
        processor.serviceAssets();
        auto status = processor.sourceStatus();
        if (status != lastStatus) {
            lastStatus = status;
            if (status.isNotEmpty())
                readout = status;
            repaint(120, 844, 870, 16);
        }
        for (int i = 0; i < 4; ++i) {
            auto data = processor.sourceData(i);
            if (displayedRevisions[size_t(i)] != data->revision) {
                displayedRevisions[size_t(i)] = data->revision;
                if (i / 2 == selectedPart)
                    repaint(i % 2 ? 764 : 16, 82, 460, 184);
            }
        }
        if (presetRevision != processor.presetRevision.load()) {
            refreshPresets();
            setDarkTheme(int(processor.state.state.getProperty("uiDark", 0)) != 0, false);
            int scope = jlimit(0, 2, int(processor.state.state.getProperty("uiScope", 2)));
            scopeView.setSource(scope);
            for (int i = 0; i < 3; ++i)
                scopeSources[size_t(i)].setToggleState(i == scope, dontSendNotification);
            patternSlot.setSelectedId(processor.activePattern() + 1, dontSendNotification);
        }
        if (rack == 0 && value(Param::arpgraphs) > 0) {
            graphView.repaint();
            if (!patternTitle.isBeingEdited())
                patternTitle.setText(processor.patternName(patternSlot.getSelectedId() - 1),
                                     dontSendNotification);
        }
        if (rack == 9)
            waveView.repaint();
        if (rack == 8)
            refreshChord();
        if (currentLayoutState() != layoutStamp)
            layout();
        for (int i = 0; i < 4; ++i)
            controls[size_t(Param(size_t(Param::o1phase) + size_t(i) * 12))]->enable(
                value(Param(size_t(Param::o1retrig) + size_t(i) * 12)) > 0 &&
                value(Param(size_t(Param::o1engine) + size_t(i) * 16)) <= 1);
        controls[size_t(Param::polyphony)]->enable(value(Param::monolegato) == 0);
        if (!pitch.isMouseButtonDown())
            pitch.setValue(processor.pitchPosition.load(), dontSendNotification);
        if (!mod.isMouseButtonDown())
            mod.setValue(processor.wheelPosition.load(), dontSendNotification);
        if (rack == 0) {
            repaint(414, 350, 520, 165);
            patternCanvas.repaint();
        }
        if (rack == 8)
            repaint(414, 366, 520, 145);
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
        shell(g, {16, 82, 460, 184}, "OSCILLATOR " + part + "1", {});
        shell(g, {764, 82, 460, 184}, "OSCILLATOR " + part + "2", {});
        shell(g, {486, 82, 268, 184}, "AMP ENVELOPE " + part, "ADSR");
        for (int slot = selectedPart * 2; slot < selectedPart * 2 + 2; ++slot)
            if (sourceViews[size_t(slot)].getSelectedId() == 2) {
                auto data = processor.sourceData(slot);
                int x = slot % 2 ? 850 : 102;
                Path path;
                for (int i = 0; i < 340; ++i) {
                    float sample = 0;
                    int mode = int(value(Param(size_t(Param::o1engine) + size_t(slot) * 16)));
                    if (mode == 1)
                        sample = data->table(
                            double(i) / 340,
                            float(value(Param(size_t(Param::o1position) + size_t(slot) * 16))),
                            .001);
                    else
                        sample =
                            data->sample(double(i) * double(data->samples[0].size() - 2) / 340, 0);
                    float y = 204 - jlimit(-1.f, 1.f, sample) * 6;
                    if (i == 0)
                        path.startNewSubPath(float(x + i), y);
                    else
                        path.lineTo(float(x + i), y);
                }
                g.setColour(green.withAlpha(.6f));
                g.strokePath(path, PathStrokeType(1));
                drawCaption(g, String(data->name), {x, 248, 340, 14}, 8, muted);
            }
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
            drawCaption(g, i == 0 ? "L" : "R", {x, 428, 15, 14}, 8, muted, Justification::centred);
            float level = processor.channelPeaks[size_t(i)].load();
            float db = 20 * std::log10(jmax(.00001f, level));
            int lit = jlimit(0, 16, int((db + 48) / 3));
            for (int j = 0; j < 16; ++j) {
                g.setColour(j < lit     ? (j > 13 ? Colour(0xffbb7c46) : green)
                            : darkTheme ? Colour(0xff354138)
                                        : Colour(0xffdee2d8));
                g.fillRoundedRectangle(float(x + 3), float(506 - j * 4), 9, 3, 1);
            }
        }

        g.setColour(screen);
        g.fillRoundedRectangle(Rectangle<float>(280, 278, 668, 258), 8);
        g.setColour(screenLine);
        g.drawRoundedRectangle(Rectangle<float>(280.5f, 278.5f, 667, 257), 8, 1);
        g.drawVerticalLine(404, 292, 523);
        drawCaption(g, "SIGNAL RACK", {292, 290, 108, 16}, 8, mint.withAlpha(.5f));
        StringArray titles{"ARPEGGIATOR", "DISTORTION", "PHASER",     "CHORUS", "EQUALIZER",
                           "REVERB",      "DELAY",      "COMPRESSOR", "CHORDS", "SAMPLE"};
        drawCaption(g,
                    (rack == 9 ||
                     (rack == 0 && value(Param::arpgraphs) > 0 && value(Param::arppattern) > 0))
                        ? String{}
                        : titles[rack],
                    {414, 289, 350, 20}, 12, screenText, Justification::centredLeft, true);

        if (rack == 9) {
            drawCaption(g, "SAMPLE / PLAYBACK", {540, 289, 300, 23}, 11, screenText);
        } else if (rack == 0 && value(Param::arppattern) > 0 && value(Param::arpgraphs) > 0) {
            drawCaption(g, "LEN", {414, 347, 31, 22}, 8, mint);
            drawCaption(g, "DIV", {497, 347, 31, 22}, 8, mint);
            drawCaption(g, "RATE", {577, 347, 36, 22}, 8, mint);
        } else if (rack == 8) {
            drawCaption(g, "TRIGGER", {414, 309, 140, 18}, 9, mint);
            drawCaption(g, "VOICING", {566, 309, 145, 18}, 9, mint);
            drawCaption(g,
                        processor.learning() >= 0 ? "Play a chord, then choose DONE"
                                                  : "Each trigger stores up to eight notes",
                        {414, 371, 510, 24}, 12, screenText);
            drawCaption(g,
                        "VOICE 1       VOICE 2       VOICE 3       VOICE 4       VOICE 5       "
                        "VOICE 6       VOICE 7       VOICE 8",
                        {414, 446, 520, 18}, 8, mint);
            drawCaption(g, "CHORDS → ARPEGGIATOR → SYNTH / MIDI OUT", {414, 490, 520, 24}, 10,
                        mint.withAlpha(.65f));
        } else if (rack == 0 && value(Param::arppattern) > 0) {
            drawCaption(g, "SWING", {600, 291, 47, 18}, 8, mint);
            drawCaption(g, "LANE / LEN", {414, 345, 87, 12}, 7, mint.withAlpha(.65f));
            StringArray labels{"NOTE", "OCTAVE", "VELOCITY", "GATE", "REPEAT", "CHANCE", "TIE"};
            for (int row = 0; row < 7; ++row)
                drawCaption(g, labels[row], {414, 359 + row * 22, 51, 19}, 8, mint);
            for (int column = 0; column < 16; ++column)
                drawCaption(g, String(stepPage * 16 + column + 1), {509 + column * 26, 345, 24, 12},
                            7, mint.withAlpha(.65f), Justification::centred);
        } else if (rack == 0) {
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
    std::array<std::unique_ptr<RackButton>, 10> rackButtons;
    std::array<ComboBox, 4> sourceViews, sourceFrame;
    std::array<TextButton, 4> sourceLoad, sourceClear;
    ComboBox patternSlot, chordTrigger, chordType;
    TextButton patternSave, patternRandom, chordLearn, chordCopy;
    std::array<Slider, 8> chordNotes;
    bool updatingChord = false;
    std::unique_ptr<FileChooser> sourceChooser;
    int previewSource = -1, previewPattern = -1, previewGraphs = -1, previewGran = -1,
        previewLink = -1;
    ComboBox presets;
    ToggleButton theme;
    TextButton previous, next;
    Keys keyboard;
    PatternCanvas patternCanvas;
    logisth_ui::GraphView graphView;
    logisth_ui::WaveView waveView;
    logisth_ui::ScopeView scopeView;
    std::array<logisth_ui::ClickArea, 4> sourceWave;
    std::array<ToggleButton, 8> graphTabs;
    std::array<ToggleButton, 3> scopeSources;
    TextButton presetFiles, graphCopy, graphPaste, graphDice, graphBankCopy;
    ComboBox waveSlot;
    Label patternTitle;
    std::unique_ptr<FileChooser> presetChooser;
    Array<File> userPresets;
    int presetRevision = -1;
    Slider pitch, mod;
    TooltipWindow tooltips;
    String readout, lastStatus;
    std::array<uint64_t, 4> displayedRevisions{};
    std::array<int, 15> layoutStamp{};
    int selectedPart = 0, rack = 0, stepPage = 0, previewSync = -1;
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
    console->setDarkTheme(scenario >= 4 && scenario != 7 && scenario != 13, false);
    console->showScenario(scenario);
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
