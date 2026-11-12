#include "PluginEditor.h"
using namespace juce;
using namespace logisth;
namespace {
const Colour bg{0xff10171e}, surface{0xff1b2631}, edge{0xff304150}, ink{0xffd8e4e9},
    accent{0xff69e1b5};
}
LogisthLook::LogisthLook() {
    setColour(Slider::textBoxTextColourId, ink);
    setColour(Slider::textBoxBackgroundColourId, bg);
    setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour(ComboBox::backgroundColourId, bg);
    setColour(ComboBox::textColourId, ink);
    setColour(ComboBox::outlineColourId, edge);
    setColour(PopupMenu::backgroundColourId, surface);
    setColour(PopupMenu::highlightedBackgroundColourId, edge);
    setColour(Label::textColourId, ink);
    setColour(GroupComponent::textColourId, accent);
    setColour(GroupComponent::outlineColourId, edge);
}
void LogisthLook::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float pos, float start,
                                   float end, Slider &) {
    auto b = Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(6);
    float r = jmin(b.getWidth(), b.getHeight()) * .5f;
    auto c = b.getCentre();
    Path track;
    track.addCentredArc(c.x, c.y, r, r, 0, start, end, true);
    g.setColour(edge);
    g.strokePath(track, PathStrokeType(3));
    Path arc;
    arc.addCentredArc(c.x, c.y, r, r, 0, start, start + pos * (end - start), true);
    g.setColour(accent);
    g.strokePath(arc, PathStrokeType(3));
    g.setColour(bg);
    g.fillEllipse(c.x - r + 5, c.y - r + 5, 2 * (r - 5), 2 * (r - 5));
    float a = start + pos * (end - start) - MathConstants<float>::halfPi;
    g.setColour(ink);
    g.drawLine(c.x + std::cos(a) * r * .4f, c.y + std::sin(a) * r * .4f,
               c.x + std::cos(a) * r * .75f, c.y + std::sin(a) * r * .75f, 2);
}
class LogisthEditor::FormPage : public Component {
    struct Field {
        String id;
        Label label;
        Slider slider;
        ComboBox menu;
        bool choice = false;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> sa;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> ca;
    };
    struct Group {
        String name;
        GroupComponent frame;
        std::vector<std::unique_ptr<Field>> fields;
    };
    std::vector<std::unique_ptr<Group>> groups;

  public:
    FormPage(LogisthProcessor &processor, int page) {
        for (const auto &spec : parameterSpecs) {
            if (spec.page != page)
                continue;
            Group *group = nullptr;
            for (auto &candidate : groups)
                if (candidate->name == spec.group)
                    group = candidate.get();
            if (!group) {
                auto g = std::make_unique<Group>();
                g->name = spec.group;
                g->frame.setText(spec.group);
                addAndMakeVisible(g->frame);
                group = g.get();
                groups.push_back(std::move(g));
            }
            auto f = std::make_unique<Field>();
            f->id = spec.id;
            f->label.setText(spec.label, dontSendNotification);
            f->label.setJustificationType(Justification::centred);
            f->label.setFont(Font(FontOptions(11.f)));
            f->label.setColour(Label::textColourId, ink.withAlpha(.75f));
            addAndMakeVisible(f->label);
            f->choice = *spec.choices;
            if (f->choice) {
                auto choices = StringArray::fromTokens(spec.choices, "|", "");
                bool destination = String(spec.id).contains("dest");
                for (int i = 0; i < choices.size(); ++i) {
                    if (destination && (i == 1 || i == 13 || i == 19))
                        f->menu.getRootMenu()->addSectionHeader(i == 1    ? "Oscillators"
                                                                : i == 13 ? "Filters"
                                                                          : "Misc");
                    f->menu.addItem(choices[i], i + 1);
                }
                f->menu.setTooltip(processor.state.getParameter(spec.id)->getName(100));
                addAndMakeVisible(f->menu);
                f->ca = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.state, spec.id, f->menu);
            } else {
                f->slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
                f->slider.setTextBoxStyle(Slider::TextBoxBelow, false, 78, 18);
                f->slider.setTooltip(processor.state.getParameter(spec.id)->getName(100));
                addAndMakeVisible(f->slider);
                f->sa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                    processor.state, spec.id, f->slider);
                f->slider.setDoubleClickReturnValue(
                    true, String(spec.id).contains("amt") ? 0 : spec.initial);
            }
            group->fields.push_back(std::move(f));
        }
        // Keep the shared controls visible before the per-part panels.
        if (page == 0)
            std::stable_sort(groups.begin(), groups.end(), [](auto &a, auto &b) {
                auto rank = [](const String &name) {
                    return name == "FILTER CONTROL"     ? 0
                           : name == "MIXER"            ? 1
                           : name.startsWith("FILTER ") ? 2
                           : name.startsWith("AMP ENV") ? 3
                           : name == "PERFORMANCE"      ? 5
                                                        : 4;
                };
                return rank(a->name) < rank(b->name);
            });
        int y = 12;
        for (size_t row = 0; row < groups.size(); row += 2) {
            int rowHeight = 0;
            for (size_t i = row; i < std::min(row + 2, groups.size()); ++i) {
                auto &g = *groups[i];
                int cols = g.name.startsWith("MISC") ? 2 : g.name.startsWith("STEP ") ? 3 : 4;
                const bool routes = g.name.startsWith("MOD ENV") || g.name.startsWith("LFO") ||
                                    g.name.startsWith("MISC");
                const int routeStart = g.name.startsWith("MOD ENV") ? 4
                                       : g.name.startsWith("LFO")   ? 6
                                                                    : 1;
                const int parameterRows = g.name.startsWith("MISC") ? 1 : (routeStart + 3) / 4;
                const int height =
                    34 + (routes ? parameterRows + 2
                                 : int((g.fields.size() + size_t(cols) - 1) / size_t(cols))) *
                             92;
                rowHeight = std::max(rowHeight, height);
                int x = 12 + int(i % 2) * 508;
                g.frame.setBounds(x, y, 496, height);
                for (size_t k = 0; k < g.fields.size(); ++k) {
                    auto &f = *g.fields[k];
                    auto cell = Rectangle<int>(x + 8 + int(k % size_t(cols)) * 120,
                                               y + 24 + int(k / size_t(cols)) * 92,
                                               cols == 2   ? 240
                                               : cols == 3 ? 160
                                                           : 120,
                                               88);
                    // Menus for long modulation destinations get half a panel each.
                    if (cols == 2)
                        cell.setX(x + 8 + int(k % 2) * 240);
                    if (cols == 3)
                        cell.setX(x + 8 + int(k % 3) * 160);
                    if (routes) {
                        if (int(k) < routeStart) {
                            cell = {x + 8 + int(k % 4) * 120, y + 24 + int(k / 4) * 92,
                                    g.name.startsWith("MISC") ? 480 : 120, 88};
                        } else {
                            int route = (int(k) - routeStart) / 2;
                            bool destination = (int(k) - routeStart) % 2 == 0;
                            cell = {x + 8 + (destination ? 96 : 0),
                                    y + 24 + (parameterRows + route) * 92, destination ? 384 : 96,
                                    88};
                        }
                    }
                    f.label.setBounds(cell.removeFromTop(18));
                    if (f.choice)
                        f.menu.setBounds(
                            cell.reduced(5).withSizeKeepingCentre(cell.getWidth() - 10, 28));
                    else
                        f.slider.setBounds(cell.reduced(3));
                }
            }
            y += rowHeight + 10;
        }
        setSize(1028, y + 8);
    }
    void updateEnabling(LogisthProcessor &p) {
        auto value = [&](const String &id) { return p.state.getRawParameterValue(id)->load(); };
        bool sync = value("sync") > 0;
        for (auto &group : groups)
            for (auto &field : group->fields) {
                auto &id = field->id;
                bool enabled = true;
                if (id.startsWith("lfo")) {
                    String base = id.substring(0, 4);
                    bool clock = sync && value(base + "free") == 0;
                    if (id.endsWith("rate"))
                        enabled = !clock;
                    if (id.endsWith("division"))
                        enabled = clock;
                }
                if (id == "phaserrate" || id == "chorusrate" || id == "arptime" ||
                    id == "delaytime" || id == "delayright")
                    enabled = !sync;
                if (id == "phaserdivision" || id == "chorusdivision" || id == "arpdivision" ||
                    id == "delaydivL" || id == "delaydivR")
                    enabled = sync;
                if (id.startsWith("o") && id.endsWith("phase"))
                    enabled = value(id.substring(0, 2) + "retrig") > 0;
                if (id == "polyphony")
                    enabled = value("monolegato") == 0;
                if (id.startsWith("step") && id.endsWith("transpose"))
                    enabled = value("arpmode") >= 8;
                field->slider.setEnabled(enabled);
                field->menu.setEnabled(enabled);
                field->label.setAlpha(enabled ? 1.f : .4f);
            }
    }
    void paint(Graphics &g) override {
        g.fillAll(bg);
    }
};
LogisthEditor::LogisthEditor(LogisthProcessor &p)
    : AudioProcessorEditor(p), processor(p),
      keyboard(p.keyboard, MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    addAndMakeVisible(tabs);
    StringArray names{"SYNTHESIS", "MODULATION", "ARPEGGIATOR", "EFFECTS"};
    for (int i = 0; i < 4; ++i) {
        pages[i] = std::make_unique<FormPage>(p, i);
        views[i] = std::make_unique<Viewport>();
        views[i]->setViewedComponent(pages[i].get(), false);
        views[i]->setScrollBarsShown(true, false);
        tabs.addTab(names[i], surface, views[i].get(), false);
    }
    presets.addItemList({"Init / analog", "Wide supersaw", "Sub bass", "Slow atmosphere",
                         "Pulse pluck", "Step sequence"},
                        1);
    presets.setTextWhenNothingSelected("Choose a factory sound...");
    presets.onChange = [this] { processor.loadPreset(presets.getSelectedId() - 1); };
    addAndMakeVisible(presets);
    presetLabel.setText("LOAD FACTORY SOUND", dontSendNotification);
    presetLabel.setFont(Font(FontOptions(10.f)));
    addAndMakeVisible(presetLabel);
    keyboard.setAvailableRange(24, 96);
    keyboard.setKeyWidth(22);
    addAndMakeVisible(keyboard);
    for (auto *wheel : {&pitchWheel, &modWheel}) {
        wheel->setSliderStyle(Slider::LinearVertical);
        wheel->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(wheel);
    }
    pitchWheel.setRange(-1, 1, .001);
    pitchWheel.setValue(0);
    pitchWheel.setTooltip("Pitch bend");
    modWheel.setRange(0, 1, .001);
    modWheel.setTooltip("Modulation wheel");
    pitchWheel.onValueChange = [this] { processor.setUiWheel(true, float(pitchWheel.getValue())); };
    pitchWheel.onDragEnd = [this] { pitchWheel.setValue(0); };
    modWheel.onValueChange = [this] { processor.setUiWheel(false, float(modWheel.getValue())); };
    setSize(1060, 900);
    for (auto &page : pages)
        page->updateEnabling(processor);
    startTimerHz(24);
}
LogisthEditor::~LogisthEditor() {
    stopTimer();
    tabs.clearTabs();
    for (auto &view : views)
        view->setViewedComponent(nullptr, false);
    for (auto &page : pages)
        page.reset();
    setLookAndFeel(nullptr);
}
void LogisthEditor::resized() {
    presetLabel.setBounds(450, 10, 300, 18);
    presets.setBounds(450, 30, 310, 30);
    tabs.setBounds(8, 78, 1044, 726);
    pitchWheel.setBounds(14, 826, 27, 60);
    modWheel.setBounds(45, 826, 27, 60);
    keyboard.setBounds(82, 822, 966, 64);
    keyboard.setLowestVisibleKey(48);
}
void LogisthEditor::timerCallback() {
    pages[size_t(tabs.getCurrentTabIndex())]->updateEnabling(processor);
    if (!pitchWheel.isMouseButtonDown())
        pitchWheel.setValue(processor.pitchPosition.load(), dontSendNotification);
    if (!modWheel.isMouseButtonDown())
        modWheel.setValue(processor.wheelPosition.load(), dontSendNotification);
    repaint(790, 8, 265, 62);
}
void LogisthEditor::paint(Graphics &g) {
    g.fillAll(bg);
    g.setColour(accent);
    g.setFont(FontOptions(32.f).withStyle("Bold"));
    g.drawText("logisth", 20, 10, 300, 40, Justification::centredLeft);
    g.setColour(ink.withAlpha(.65f));
    g.setFont(FontOptions(11.f));
    g.drawText("DUAL LAYER / VIRTUAL ANALOG", 22, 48, 380, 20, Justification::centredLeft);
    g.drawText("VOICES " + String(processor.voices.load()) + " / OUTPUT", 795, 12, 245, 18,
               Justification::centredLeft);
    for (int ch = 0; ch < 2; ++ch) {
        g.setColour(edge);
        g.fillRoundedRectangle(810, 36 + ch * 14, 230, 7, 3);
        g.setColour(ink);
        g.drawText(ch == 0 ? "L" : "R", 795, 31 + ch * 14, 12, 16, Justification::centredLeft);
        g.setColour(accent);
        g.fillRoundedRectangle(810, 36 + ch * 14,
                               230 * jlimit(0.f, 1.f, processor.channelPeaks[size_t(ch)].load()), 7,
                               3);
    }
}
Image LogisthEditor::snapshotPage(int page) {
    tabs.setCurrentTabIndex(jlimit(0, 3, page));
    pages[size_t(jlimit(0, 3, page))]->updateEnabling(processor);
    return pages[size_t(jlimit(0, 3, page))]->createComponentSnapshot(
        pages[size_t(jlimit(0, 3, page))]->getLocalBounds());
}
