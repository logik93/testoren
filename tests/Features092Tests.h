#pragma once
#include "ProcessorPhaseTests.h"
namespace features092_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool v, const char *text) {
        if (!v)
            throw std::runtime_error(text);
    };
    LogisthProcessor p;
    for (int env = 1; env <= 2; ++env)
        for (auto suffix : {"a", "d", "s", "r", "amt1", "amt2", "dest1", "dest2"})
            check(p.state.getRawParameterValue("env" + String(env) + suffix)->load() == 0,
                  "fresh modulation envelopes and routes are all zero");
    std::vector<std::pair<String, float>> settings;
    for (int slot = 0; slot < 4; ++slot) {
        auto prefix = "o" + String(slot + 1);
        processor_phase_test::configure(p, slot, 0, true, 0);
        auto before = processor_phase_test::render(p, 8192, true);
        gain_test::set(p, prefix + "fmdepth", 2);
        p.reset();
        auto after = processor_phase_test::render(p, 8192, true);
        double difference = 0;
        for (int i = 0; i < before.getNumSamples(); ++i)
            difference += std::abs(before.getSample(0, i) - after.getSample(0, i));
        check(difference > 1, "FM parameter reaches each actual oscillator in the processor");
        for (auto pair : {std::pair<const char *, float>{"fmdepth", 1.7f},
                          {"fmratio", 2.5f},
                          {"amdepth", .4f},
                          {"amratio", 1.5f},
                          {"pulsewidth", .31f},
                          {"pwmdepth", .16f},
                          {"pwmrate", 3.4f},
                          {"stretchmode", 3},
                          {"stretch", 2.5f},
                          {"interpolation", 3}})
            settings.emplace_back(prefix + pair.first, pair.second);
    }
    for (auto pair : {std::pair<const char *, float>{"eqmode", 1},
                      {"eqoutput", -2.5f},
                      {"eqautogain", 1},
                      {"reverbdiffusion", .43f}})
        settings.emplace_back(pair.first, pair.second);
    for (int band = 0; band < 10; ++band)
        settings.emplace_back("eqband" + String(band), float(band) - 4);
    for (auto &v : settings)
        gain_test::set(p, v.first, v.second);
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "new feature preset saves");
    LogisthProcessor restored;
    check(restored.loadPresetFile(preset.getFile()).wasOk(), "new feature preset loads");
    for (auto &v : settings)
        check(std::abs(restored.state.getRawParameterValue(v.first)->load() - v.second) < 1e-4,
              "all new source, reverb and EQ values persist in presets");
    gain_test::cleanSine(p);
    gain_test::set(p, "eqon", 1);
    gain_test::set(p, "eqmode", 1);
    gain_test::set(p, "eqband5", 6);
    auto boost = gain_test::measure(p, 48000, {83}).dbPeak(); // near 1 kHz
    gain_test::set(p, "eqautogain", 1);
    auto compensated = gain_test::measure(p, 48000, {83}).dbPeak();
    check(boost > 5.8 && compensated < .1 && compensated > -.2,
          "graphic EQ boost and optional Auto Gain reach the processor output");
    std::unique_ptr<AudioProcessorEditor> ui(restored.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    editor->snapshotPage(22);
    auto *graph = console->findChildWithID("eq.graph");
    check(graph && graph->isVisible(), "graphic EQ response is visible");
    auto event = [&](Point<float> at) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys(ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, graph,
                          graph, Time::getCurrentTime(), at, Time::getCurrentTime(), 1, false);
    };
    float x = 5 + float(std::log(1000. / 20) / std::log(1000.)) * (graph->getWidth() - 10);
    float y = float(graph->getHeight()) / 2;
    float initial = restored.state.getRawParameterValue("eqband5")->load();
    graph->mouseDown(event({x, y}));
    check(restored.state.getRawParameterValue("eqband5")->load() == initial,
          "Auto Gain does not make a band jump on initial click");
    graph->mouseDrag(event({x, y - 10}));
    graph->mouseUp(event({x, y - 10}));
    check(restored.state.getRawParameterValue("eqband5")->load() > initial + 5,
          "graphic drag changes the bound band parameter");
    graph->mouseDoubleClick(event({x, y}));
    check(restored.state.getRawParameterValue("eqband5")->load() == 0,
          "graphic double-click resets exactly the selected band");
    editor->snapshotPage(21);
    check(console->findChildWithID("eqbass")->isVisible() &&
              !console->findChildWithID("eqband5")->isVisible(),
          "Legacy exposes shelves and hides graphic faders");
    editor->snapshotPage(20);
    for (int part = 0; part < 2; ++part) {
        dynamic_cast<Button *>(console->findChildWithID("part." + String(part)))->onClick();
        for (int side = 0; side < 2; ++side) {
            int slot = part * 2 + side;
            auto prefix = "o" + String(slot + 1);
            auto *page =
                dynamic_cast<ComboBox *>(console->findChildWithID("source.view." + String(slot)));
            check(page != nullptr, "oscillator page selector exists");
            page->setSelectedId(7, sendNotificationSync);
            check(console->findChildWithID(prefix + "fmdepth")->isVisible(),
                  "each periodic source exposes FM controls");
        }
    }
    check(editor->validateLayout().isEmpty(), "all new controls are reachable without overlaps");
    std::cout << "PASS 0.9.2 integration: zero MOD ENV, source DSP bindings, EQ audio/gestures, "
                 "new parameter presets and conditional UI\n";
}
} // namespace features092_test
