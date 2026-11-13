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
class WaveView : public Component, public FileDragAndDropTarget {
    LogisthProcessor &p;
    LogisthColours &colours;
    FontMaker font;
    int slot = 0, displayMode = -1;
    bool draggingFile = false;
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
                auto peaks = source.rangePeaks(ch, a, b);
                minima[size_t(ch)][size_t(x)] = peaks[0];
                maxima[size_t(ch)][size_t(x)] = peaks[1];
            }
        cachedRevision = source.revision;
    }
    bool wavetable() const {
        return (displayMode >= 0 ? displayMode : int(raw(p, "o" + String(slot + 1) + "engine"))) ==
               1;
    }
    void seek(Point<float> at) {
        auto data = p.sourceData(slot);
        float where = float(std::clamp(wavetable() ? double(at.x - 12) / (getWidth() - 24)
                                                   : offset + at.x / getWidth() / zoom,
                                       0., 1.));
        if (wavetable())
            where = data->framePosition(data->frameIndex(where));
        change(p, "o" + String(slot + 1) + "position", where);
        repaint();
    }
    void paintWavetable(Graphics &g, const logisth::SourceData &data) {
        auto telemetry = p.playback[size_t(slot)].read();
        float position = raw(p, "o" + String(slot + 1) + "position");
        const bool live = telemetry.active && telemetry.revision == data.revision;
        int selected = data.frameIndex(live ? telemetry.position : position);
        g.fillAll(colours.screen);
        g.setColour(colours.screenText);
        g.setFont(font(10));
        g.drawFittedText(String(data.name) + "  /  " + String(data.frames) + " frames x " +
                             String(data.frameSize) + " samples",
                         0, 0, getWidth(), 21, Justification::centredLeft, 1);
        const float width = float(getWidth() - 120);
        auto wave = [&](int frame, bool highlight) {
            const float depth = data.framePosition(frame);
            Path path;
            for (int i = 0; i <= 160; ++i) {
                float x = 15 + depth * 80 + width * i / 160.f;
                float y = 135 - depth * 65 -
                          std::clamp(data.frameSample(frame, double(i) / 160), -1.f, 1.f) * 25;
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            g.setColour(highlight ? colours.mint : colours.mint.withAlpha(.18f));
            g.strokePath(path, PathStrokeType(highlight ? 2.2f : .8f));
        };
        const int stride = std::max(1, (data.frames + 31) / 32);
        for (int i = data.frames - 1; i >= 0; i -= stride)
            wave(i, false);
        wave(selected, true);
        g.setColour(colours.mint.withAlpha(.14f));
        g.fillRoundedRectangle(12, 166, float(getWidth() - 24), 9, 3);
        g.setColour(colours.mint);
        const float x = 12 + data.framePosition(selected) * (getWidth() - 24);
        g.fillRoundedRectangle(x - 2, 163, 4, 15, 2);
        g.setFont(font(12));
        g.setColour(colours.screenText);
        g.drawText(String(selected + 1) + " / " + String(data.frames), getWidth() - 105, 123, 96,
                   28, Justification::centredRight);
        g.setFont(font(10));
        g.setColour(colours.mint.withAlpha(.8f));
        g.drawText("WT POS  /  drag: frame  /  wheel: previous-next  /  drop: import", 0, 182,
                   getWidth(), 18, Justification::centredLeft);
        if (draggingFile) {
            g.setColour(colours.mint);
            g.drawRect(getLocalBounds().reduced(1), 2);
        }
    }

  public:
    std::function<void(int, const File &)> onFileDropped;
    bool isInterestedInFileDrag(const StringArray &files) override {
        return files.size() == 1 && File(files[0]).hasFileExtension("wav;aif;aiff;flac");
    }
    void fileDragEnter(const StringArray &files, int, int) override {
        draggingFile = isInterestedInFileDrag(files);
        repaint();
    }
    void fileDragExit(const StringArray &) override {
        draggingFile = false;
        repaint();
    }
    void filesDropped(const StringArray &files, int, int) override {
        draggingFile = false;
        if (isInterestedInFileDrag(files) && onFileDropped)
            onFileDropped(slot, File(files[0]));
        repaint();
    }
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
    void setMode(int mode) {
        displayMode = mode;
    }
    int selectedSlot() const {
        return slot;
    }
    void paint(Graphics &g) override {
        auto data = p.sourceData(slot);
        if (wavetable()) {
            paintWavetable(g, *data);
            return;
        }
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
        const bool granular =
            (displayMode >= 0 ? displayMode : int(raw(p, "o" + String(slot + 1) + "engine"))) == 3;
        int mode = int(raw(p, "o" + String(slot + 1) + "gtrans"));
        static const std::vector<int64_t> noMarkers;
        const auto &markers = !granular ? noMarkers : mode == 1 ? data->regions : data->transients;
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
        g.drawText("Drop sample here  /  click: Start  /  wheel: zoom  /  Shift+wheel: pan", 0, 181,
                   getWidth(), 18, Justification::centredLeft);
        if (draggingFile) {
            g.setColour(colours.mint.withAlpha(.12f));
            g.fillRect(getLocalBounds());
            g.setColour(colours.mint);
            g.drawRect(getLocalBounds().reduced(1), 2);
            g.fillRoundedRectangle(float(getWidth() / 2 - 120), 70, 240, 38, 4);
            g.setColour(colours.screen);
            g.setFont(font(13));
            g.drawText("Load sample into " + StringArray{"A1", "A2", "B1", "B2"}[slot],
                       getWidth() / 2 - 116, 72, 232, 34, Justification::centred);
        }
    }
    void mouseDown(const MouseEvent &e) override {
        if (e.position.y > 24 && e.position.y < 177)
            seek(e.position);
    }
    void mouseDrag(const MouseEvent &e) override {
        seek(e.position);
    }
    void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
        if (wheel.deltaY == 0)
            return;
        if (wavetable()) {
            auto data = p.sourceData(slot);
            auto id = "o" + String(slot + 1) + "position";
            change(p, id,
                   data->framePosition(data->frameIndex(raw(p, id)) + (wheel.deltaY > 0 ? 1 : -1)));
            repaint();
            return;
        }
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
