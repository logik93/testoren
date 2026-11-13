#pragma once
#include "PluginEditor.h"
namespace logisth_ui {
using namespace juce;
using FontMaker = std::function<Font(float)>;
inline float raw(LogisthProcessor &p, const String &id) {
    return p.state.getRawParameterValue(id)->load();
}
inline void change(LogisthProcessor &p, const String &id, float value) {
    if (auto *parameter = p.state.getParameter(id)) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}
class ClickArea : public Component, public SettableTooltipClient {
  public:
    std::function<void()> onClick;
    ClickArea() {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    void mouseDown(const MouseEvent &) override {
        if (onClick)
            onClick();
    }
};
class GraphView : public Component {
    LogisthProcessor &p;
    LogisthColours &colours;
    FontMaker font;
    int lane = 0, dragColumn = -1;
    bool draggingLength = false;
    RangedAudioParameter *gesture = nullptr;
    TextEditor input;
    int inputColumn = 0;
    static const char *suffix(int lane) {
        static const char *names[] = {"note",     "random", "octave", "pitch",
                                      "velocity", "gate",   "late",   "harmony"};
        return names[std::clamp(lane, 0, 7)];
    }
    String id(int column) const {
        return "g" + String(column + 1) + suffix(lane);
    }
    static String noteName(int value) {
        static const char *names[] = {"UP", "DN", "UD", "DU", "U+D", "D+U", "FLO", "FHI"};
        return value < 0    ? "-"
               : value <= 7 ? String(value + 1)
                            : String(names[std::clamp(value - 8, 0, 7)]);
    }
    Rectangle<float> cell(int column) const {
        return {float(column) * getWidth() / 32, 7, float(getWidth()) / 32 - 1, 107};
    }
    int column(Point<float> at) const {
        return std::clamp(int(at.x * 32 / getWidth()), 0, 31);
    }
    void finish() {
        if (gesture) {
            gesture->endChangeGesture();
            gesture = nullptr;
        }
    }
    void drawValue(Point<float> at) {
        int col = column(at);
        auto *parameter = p.state.getParameter(id(col));
        if (gesture != parameter) {
            finish();
            gesture = parameter;
            gesture->beginChangeGesture();
        }
        auto range = parameter->getNormalisableRange();
        float value = std::round(range.start + (range.end - range.start) *
                                                   (1 - jlimit(0.f, 1.f, (at.y - 8) / 105)));
        gesture->setValueNotifyingHost(parameter->convertTo0to1(value));
        dragColumn = col;
    }
    void lengthAt(Point<float> at) {
        int value = std::clamp(int(std::round(at.x * 32 / getWidth())), 1, 32);
        if (raw(p, "glink") > 0)
            for (int i = 0; i < 8; ++i)
                change(p, "glen" + String(i), float(value));
        else
            change(p, "glen" + String(lane), float(value));
    }
    void editValue(int col) {
        finish();
        inputColumn = col;
        float value = raw(p, id(col));
        input.setText(lane == 0 ? noteName(int(value)) : String(int(value)), false);
        int x = std::clamp(int(cell(col).getX()) - 20, 0, getWidth() - 64);
        input.setBounds(x, 44, 64, 25);
        input.setFont(font(12));
        input.setColour(TextEditor::backgroundColourId, colours.screen);
        input.setColour(TextEditor::textColourId, colours.screenText);
        input.setVisible(true);
        input.grabKeyboardFocus();
        input.selectAll();
    }
    void commit() {
        if (!input.isVisible())
            return;
        String text = input.getText().trim().toUpperCase();
        float value = float(text.getDoubleValue());
        if (lane == 0) {
            if (text == "-" || text == "REST")
                value = -1;
            else {
                bool mode = false;
                for (int i = 8; i < 16; ++i)
                    if (noteName(i) == text) {
                        value = float(i);
                        mode = true;
                        break;
                    }
                if (!mode)
                    value -= 1;
            }
        }
        auto *parameter = p.state.getParameter(id(inputColumn));
        auto range = parameter->getNormalisableRange();
        input.setVisible(false);
        change(p, id(inputColumn), jlimit(range.start, range.end, value));
    }

  public:
    GraphView(LogisthProcessor &processor, LogisthColours &palette, FontMaker f)
        : p(processor), colours(palette), font(std::move(f)) {
        setComponentID("graph.canvas");
        setName("Arpeggiator graph editor");
        addChildComponent(input);
        input.onReturnKey = [this] { commit(); };
        input.onFocusLost = [this] { commit(); };
        input.onEscapeKey = [this] { input.setVisible(false); };
    }
    ~GraphView() override {
        finish();
        input.onFocusLost = nullptr;
    }
    void setLane(int next) {
        finish();
        input.setVisible(false);
        lane = std::clamp(next, 0, 7);
        repaint();
    }
    int selectedLane() const {
        return lane;
    }
    static bool ownsParameter(const String &name) {
        return name.startsWith("gdegree") || name.startsWith("gprevent") ||
               (name.length() > 1 && name[0] == 'g' && CharacterFunctions::isDigit(name[1]));
    }
    void paint(Graphics &g) override {
        g.fillAll(colours.screen);
        int length = int(raw(p, "glen" + String(raw(p, "glink") > 0 ? 0 : lane)));
        int playhead = p.graphPlayheads[size_t(lane)].load();
        g.setColour(colours.screenLine);
        for (int i = 0; i < 5; ++i)
            g.drawHorizontalLine(8 + i * 26, 0, float(getWidth()));
        g.setColour(colours.mint);
        g.fillRect(0.f, 0.f, float(getWidth()) * length / 32, 3.f);
        g.setFont(font(8));
        for (int col = 0; col < 32; ++col) {
            auto b = cell(col);
            auto *parameter = p.state.getParameter(id(col));
            auto range = parameter->getNormalisableRange();
            float value = raw(p, id(col)),
                  normal = (value - range.start) / std::max(1.f, range.end - range.start),
                  zero = (0 - range.start) / std::max(1.f, range.end - range.start);
            float y = b.getBottom() - normal * b.getHeight();
            float base = range.start < 0 ? b.getBottom() - zero * b.getHeight() : b.getBottom();
            bool muted = raw(p, "g" + String(col + 1) + "mute") > 0;
            g.setColour(colours.mint.withAlpha(col >= length ? .1f : muted ? .15f : .5f));
            if (!(lane == 0 && value < 0))
                g.fillRect(Rectangle<float>(b.getX() + 1, std::min(y, base), b.getWidth() - 2,
                                            std::max(1.f, std::abs(base - y))));
            g.setColour(colours.screenText.withAlpha(col >= length ? .25f : 1.f));
            g.drawText(lane == 0 ? noteName(int(value)) : String(int(value)), b.withHeight(14),
                       Justification::centred);
            if (col == playhead) {
                g.setColour(colours.mint);
                g.drawRect(b, 1.2f);
            }
            if (lane == 0 && raw(p, "g" + String(col + 1) + "reset") > 0) {
                g.setColour(colours.mint);
                Path arrow;
                arrow.startNewSubPath(b.getCentreX(), 96);
                arrow.lineTo(b.getCentreX(), 109);
                arrow.lineTo(b.getCentreX() - 3, 104);
                arrow.startNewSubPath(b.getCentreX(), 109);
                arrow.lineTo(b.getCentreX() + 3, 104);
                g.strokePath(arrow, PathStrokeType(1.4f));
            }
            if (lane == 0) {
                g.setColour(muted ? Colour(0xffbd7763) : colours.screenLine);
                g.fillRoundedRectangle(b.getX() + 2, 120, b.getWidth() - 3, 12, 2);
            }
        }
        if (lane == 3) {
            for (int i = 0; i < 7; ++i) {
                bool on = raw(p, "gdegree" + String(i)) > 0;
                g.setColour(on ? colours.mint : colours.screenLine);
                g.fillRoundedRectangle(float(i * 50), 119, 44, 15, 2);
                g.setColour(on ? colours.screen : colours.screenText);
                g.drawText(String(i + 1), i * 50, 119, 44, 15, Justification::centred);
            }
        }
        if (lane != 0 && lane != 3) {
            g.setColour(colours.mint.withAlpha(.6f));
            g.drawText("Drag: draw  /  right-click: value  /  top line: length", 0, 118, getWidth(),
                       16, Justification::centredLeft);
        }
        static const char *pcs[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};
        for (int i = 0; i < 12; ++i) {
            int rule = int(raw(p, "gprevent" + String(i)));
            float width = float(getWidth()) / 12;
            Rectangle<float> b(float(i) * width, 142, width - 2, 16);
            g.setColour(rule == 0 ? colours.screenLine : Colour(0xff8b5148));
            g.fillRoundedRectangle(b, 2);
            g.setColour(colours.screenText);
            g.drawText(String(pcs[i]) + (rule == 1   ? " x"
                                         : rule == 2 ? " >"
                                         : rule == 3 ? " <"
                                                     : ""),
                       b, Justification::centred);
        }
    }
    void mouseDown(const MouseEvent &e) override {
        int col = column(e.position);
        if (e.position.y >= 141) {
            int pc = std::clamp(int(e.position.x * 12 / getWidth()), 0, 11);
            String key = "gprevent" + String(pc);
            change(p, key, float((int(raw(p, key)) + 1) % 4));
            repaint();
            return;
        }
        if (e.position.y >= 118) {
            if (lane == 0) {
                String key = "g" + String(col + 1) + "mute";
                change(p, key, raw(p, key) > 0 ? 0 : 1);
            } else if (lane == 3) {
                int degree = std::clamp(int(e.position.x) / 50, 0, 6);
                String key = "gdegree" + String(degree);
                change(p, key, raw(p, key) > 0 ? 0 : 1);
            }
            repaint();
            return;
        }
        if (e.mods.isAltDown() && lane == 0) {
            String key = "g" + String(col + 1) + "reset";
            change(p, key, raw(p, key) > 0 ? 0 : 1);
            repaint();
            return;
        }
        if (e.mods.isRightButtonDown() || e.getNumberOfClicks() > 1) {
            editValue(col);
            return;
        }
        draggingLength = e.position.y < 5;
        if (draggingLength)
            lengthAt(e.position);
        else
            drawValue(e.position);
        repaint();
    }
    void mouseDrag(const MouseEvent &e) override {
        if (input.isVisible())
            return;
        if (draggingLength)
            lengthAt(e.position);
        else if (e.position.y < 118)
            drawValue(e.position);
        repaint();
    }
    void mouseUp(const MouseEvent &) override {
        finish();
        draggingLength = false;
    }
    void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
        int col = column(e.position);
        auto *parameter = p.state.getParameter(id(col));
        auto range = parameter->getNormalisableRange();
        change(p, id(col),
               jlimit(range.start, range.end, raw(p, id(col)) + (wheel.deltaY > 0 ? 1.f : -1.f)));
        repaint();
    }
    void visibilityChanged() override {
        if (!isVisible()) {
            finish();
            input.setVisible(false);
        }
    }
};
class WaveView : public Component {
    LogisthProcessor &p;
    LogisthColours &colours;
    FontMaker font;
    int slot = 0;
    double zoom = 1, offset = 0;
    uint64_t cachedRevision = 0;
    std::array<std::array<float, 520>, 2> minima{}, maxima{};
    void cache(const logisth::SourceData &source) {
        size_t size = source.samples[0].size();
        for (int ch = 0; ch < 2; ++ch)
            for (int x = 0; x < 520; ++x) {
                size_t a = std::min(size - 1,
                                    size_t((offset + double(x) / 520 / zoom) * double(size))),
                       b = std::min(size,
                                    std::max(a + 1, size_t((offset + double(x + 1) / 520 / zoom) *
                                                           double(size))));
                float lo = 0, hi = 0;
                for (size_t i = a; i < b; ++i) {
                    float sample = source.samples[size_t(ch)][i];
                    lo = std::min(lo, sample);
                    hi = std::max(hi, sample);
                }
                minima[size_t(ch)][size_t(x)] = lo;
                maxima[size_t(ch)][size_t(x)] = hi;
            }
        cachedRevision = source.revision;
    }
    void seek(Point<float> at) {
        double where = std::clamp(offset + at.x / getWidth() / zoom, 0., 1.);
        change(p, "o" + String(slot + 1) + "position", float(where));
    }

  public:
    WaveView(LogisthProcessor &processor, LogisthColours &palette, FontMaker f)
        : p(processor), colours(palette), font(std::move(f)) {
        setComponentID("sample.waveform");
        setName("Sample waveform and playback position");
    }
    void setSlot(int value) {
        slot = std::clamp(value, 0, 3);
        zoom = 1;
        offset = 0;
        cachedRevision = 0;
        repaint();
    }
    int selectedSlot() const {
        return slot;
    }
    void paint(Graphics &g) override {
        auto data = p.sourceData(slot);
        if (data->revision != cachedRevision)
            cache(*data);
        g.fillAll(colours.screen);
        g.setFont(font(10));
        g.setColour(colours.screenText);
        g.drawText(String::fromUTF8(data->name.c_str()) + "  /  " + String(data->sampleRate, 0) +
                       " Hz  /  " + String(data->samples[0].size() / data->sampleRate, 2) + " s",
                   0, 0, getWidth(), 20, Justification::centredLeft);
        auto xAt = [&](double position) { return float((position - offset) * zoom * getWidth()); };
        for (int ch = 0; ch < 2; ++ch) {
            float centre = ch == 0 ? 65.f : 139.f;
            g.setColour(colours.screenLine);
            g.drawHorizontalLine(int(centre), 0, float(getWidth()));
            g.setColour(colours.mint.withAlpha(.75f));
            for (int x = 0; x < getWidth(); ++x) {
                int index = std::clamp(x * 520 / getWidth(), 0, 519);
                g.drawVerticalLine(x, centre - maxima[size_t(ch)][size_t(index)] * 29,
                                   centre - minima[size_t(ch)][size_t(index)] * 29);
            }
            g.setColour(colours.screenText);
            g.drawText(ch ? "R" : "L", 2, int(centre) - 30, 16, 14, Justification::centredLeft);
        }
        int mode = int(raw(p, "o" + String(slot + 1) + "gtrans"));
        const auto &markers = mode == 1 ? data->regions : data->transients;
        g.setColour(colours.screenText.withAlpha(.3f));
        for (auto marker : markers) {
            float x = xAt(double(marker) / double(data->samples[0].size() - 1));
            if (x >= 0 && x < getWidth())
                g.drawVerticalLine(int(x), 25, 176);
        }
        float start = raw(p, "o" + String(slot + 1) + "position"),
              end = raw(p, "o" + String(slot + 1) + "end");
        g.setColour(colours.mint.withAlpha(.15f));
        g.fillRect(Rectangle<float>(xAt(start), 25, std::max(1.f, xAt(end) - xAt(start)), 151)
                       .getIntersection(Rectangle<float>(0, 25, float(getWidth()), 151)));
        auto telemetry = p.playback[size_t(slot)].read();
        if (telemetry.revision == data->revision) {
            for (int i = 0; i < telemetry.grainCount; ++i) {
                float x = xAt(telemetry.grains[size_t(i)]);
                if (x >= 0 && x < getWidth()) {
                    g.setColour(colours.mint.withAlpha(.4f));
                    g.drawVerticalLine(int(x), 27, 175);
                }
            }
            if (telemetry.active) {
                float x = xAt(telemetry.position);
                g.setColour(colours.screenText);
                if (x >= 0 && x < getWidth())
                    g.drawLine(x, 24, x, 177, 2);
            }
        }
        g.setColour(colours.mint.withAlpha(.65f));
        g.drawText("Latest voice  /  click: Start  /  wheel: zoom  /  Shift+wheel: pan", 0, 181,
                   getWidth(), 18, Justification::centredLeft);
    }
    void mouseDown(const MouseEvent &e) override {
        if (e.position.y > 24 && e.position.y < 177)
            seek(e.position);
    }
    void mouseDrag(const MouseEvent &e) override {
        seek(e.position);
    }
    void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
        if (e.mods.isShiftDown())
            offset = std::clamp(offset - wheel.deltaY * .2 / zoom, 0., 1 - 1 / zoom);
        else {
            double centre = offset + e.position.x / getWidth() / zoom;
            zoom = std::clamp(zoom * (wheel.deltaY > 0 ? 1.5 : 1 / 1.5), 1., 64.);
            offset = std::clamp(centre - e.position.x / getWidth() / zoom, 0., 1 - 1 / zoom);
        }
        cachedRevision = 0;
        repaint();
    }
};
class ScopeView : public Component {
    LogisthProcessor &p;
    LogisthColours &colours;
    FontMaker font;
    int selected = 2;
    std::array<float, 2048> buffer{};

  public:
    ScopeView(LogisthProcessor &processor, LogisthColours &palette, FontMaker f)
        : p(processor), colours(palette), font(std::move(f)) {
        setComponentID("output.scope");
        setName("Oscilloscope");
    }
    void setSource(int source) {
        selected = std::clamp(source, 0, 2);
        repaint();
    }
    int source() const {
        return selected;
    }
    void paint(Graphics &g) override {
        p.scope.read(selected, buffer.data(), int(buffer.size()));
        g.setColour(colours.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5);
        g.setColour(colours.screenLine);
        g.drawHorizontalLine(getHeight() / 2, 4, float(getWidth() - 4));
        float peak = 0;
        for (float value : buffer)
            peak = std::max(peak, std::abs(value));
        int start = 1024;
        for (int i = 1024; i > 0; --i)
            if (buffer[size_t(i - 1)] <= 0 && buffer[size_t(i)] > 0) {
                start = i;
                break;
            }
        Path path;
        for (int x = 0; x < getWidth() - 8; ++x) {
            int index = start + x * 512 / std::max(1, getWidth() - 8);
            float y = getHeight() * .5f -
                      buffer[size_t(index)] / std::max(.02f, peak) * (getHeight() - 30) * .5f;
            if (x == 0)
                path.startNewSubPath(4.f, y);
            else
                path.lineTo(float(x + 4), y);
        }
        g.setColour(colours.mint);
        g.strokePath(path, PathStrokeType(1.3f));
        g.setFont(font(8));
        g.setColour(colours.screenText);
        g.drawText(selected == 0   ? "A / pre FX"
                   : selected == 1 ? "B / pre FX"
                                   : "OUT / L+R",
                   7, 2, getWidth() - 14, 15, Justification::centredLeft);
        g.drawText("Peak " + String(peak, 3), 7, getHeight() - 16, getWidth() - 14, 14,
                   Justification::centredRight);
    }
};
} // namespace logisth_ui
