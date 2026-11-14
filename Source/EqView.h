#pragma once
namespace logisth_ui {
class EqView : public juce::Component, public juce::SettableTooltipClient {
    LogisthProcessor &processor;
    const LogisthColours &theme;
    std::function<int()> mode;
    int dragging = -1;
    float dragY = 0, dragValue = 0;
    juce::RangedAudioParameter *gesture = nullptr;
    float value(const juce::String &id) const {
        return processor.state.getRawParameterValue(id)->load();
    }
    logisth::EqualizerParams settings() const {
        logisth::EqualizerParams p;
        p.eqMode = mode();
        p.eqAutoGain = value("eqautogain") > 0;
        p.eqOutput = value("eqoutput");
        for (int i = 0; i < 10; ++i)
            p.eqBands[size_t(i)] = value("eqband" + juce::String(i));
        return p;
    }
    double response(const logisth::EqualizerParams &p, double hz, bool output = true) const {
        return logisth::Equalizer::response(p, value("eqbass"), value("eqbassfreq"),
                                            value("eqtreble"), value("eqtreblefreq"), hz,
                                            std::max(44100., processor.getSampleRate()), output);
    }
    float x(double hz) const {
        return 5 + float(std::log(hz / 20) / std::log(1000.)) * (getWidth() - 10);
    }
    float y(double db) const {
        return float(getHeight()) * .5f - float(db) / 48 * (getHeight() - 10);
    }
    int nearest(float px) const {
        int best = 0;
        for (int i = 1; i < 10; ++i)
            if (std::abs(px - x(logisth::Equalizer::frequencies[size_t(i)])) <
                std::abs(px - x(logisth::Equalizer::frequencies[size_t(best)])))
                best = i;
        return best;
    }
    void finish() {
        if (gesture)
            gesture->endChangeGesture();
        gesture = nullptr;
        dragging = -1;
    }

  public:
    EqView(LogisthProcessor &p, const LogisthColours &c, std::function<int()> m)
        : processor(p), theme(c), mode(std::move(m)) {
        setComponentID("eq.graph");
        setTooltip("Drag bands vertically; double-click resets a band to 0 dB.");
    }
    ~EqView() override {
        finish();
    }
    void paint(juce::Graphics &g) override {
        g.setColour(theme.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.setColour(theme.screenLine);
        for (double hz : {100., 1000., 10000.})
            g.drawVerticalLine(int(x(hz)), 3, float(getHeight() - 3));
        for (double db : {-12., 0., 12.})
            g.drawHorizontalLine(int(y(db)), 3, float(getWidth() - 3));
        auto p = settings();
        double compensation = 0;
        if (p.eqAutoGain)
            for (int i = 0; i < 96; ++i)
                compensation =
                    std::max(compensation, response(p, 20 * std::pow(1000., i / 95.), false));
        juce::Path curve;
        for (int i = 0; i < 256; ++i) {
            double hz = 20 * std::pow(1000., i / 255.);
            float xx = x(hz),
                  yy = juce::jlimit(2.f, float(getHeight() - 2), y(response(p, hz) - compensation));
            if (i == 0)
                curve.startNewSubPath(xx, yy);
            else
                curve.lineTo(xx, yy);
        }
        g.setColour(theme.green);
        g.strokePath(curve, juce::PathStrokeType(1.8f));
        if (p.eqMode == 1)
            for (int i = 0; i < 10; ++i) {
                g.setColour(i == dragging ? theme.mint : theme.green);
                g.fillEllipse(x(logisth::Equalizer::frequencies[size_t(i)]) - 3,
                              y(p.eqBands[size_t(i)] + p.eqOutput - compensation) - 3, 6, 6);
            }
        theme.drawCaption(g, "20 Hz", {6, getHeight() - 17, 50, 14}, 8, theme.muted);
        theme.drawCaption(g, "20 kHz", {getWidth() - 64, getHeight() - 17, 58, 14}, 8, theme.muted,
                          juce::Justification::centredRight);
    }
    void mouseDown(const juce::MouseEvent &e) override {
        finish();
        if (mode() != 1)
            return;
        dragging = nearest(e.position.x);
        gesture = processor.state.getParameter("eqband" + juce::String(dragging));
        dragY = e.position.y;
        dragValue = value("eqband" + juce::String(dragging));
        gesture->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent &e) override {
        if (!gesture)
            return;
        float db =
            juce::jlimit(-18.f, 18.f, dragValue + (dragY - e.position.y) * 48 / (getHeight() - 10));
        gesture->setValueNotifyingHost(gesture->convertTo0to1(db));
        repaint();
    }
    void mouseUp(const juce::MouseEvent &) override {
        finish();
    }
    void mouseDoubleClick(const juce::MouseEvent &e) override {
        finish();
        if (mode() != 1)
            return;
        auto *p = processor.state.getParameter("eqband" + juce::String(nearest(e.position.x)));
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(0));
        p->endChangeGesture();
        repaint();
    }
};
} // namespace logisth_ui
