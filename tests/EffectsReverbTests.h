#pragma once
#include "GainCalibration.h"
#include "PluginEditor.h"
namespace effects_reverb_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *text) {
        if (!ok)
            throw std::runtime_error(text);
    };
    std::vector<float> previous;
    LogisthProcessor processor;
    for (int type = 0; type < 5; ++type) {
        gain_test::cleanSine(processor);
        gain_test::set(processor, "reverbon", 1);
        gain_test::set(processor, "reverbtype", float(type));
        gain_test::set(processor, "reverb", .75f);
        AudioBuffer<float> audio;
        gain_test::measure(processor, 48000, {69}, 1, 2, &audio);
        std::vector<float> current(audio.getReadPointer(0),
                                   audio.getReadPointer(0) + audio.getNumSamples());
        if (type > 0) {
            double delta = 0;
            for (size_t i = 48000; i < current.size(); ++i)
                delta += std::abs(current[i] - previous[i]);
            check(delta > 1, "each reverb type reaches the audio processor and changes its output");
        }
        previous = std::move(current);
    }
    {
        Effects fx;
        fx.prepare(48000);
        EffectsParams p;
        p.reverb = true;
        p.reverbMix = 0;
        std::array<float, 512> left{}, right{}, mod{};
        for (int type = 0; type < 5; ++type) {
            p.reverbType = type;
            for (int i = 0; i < 512; ++i) {
                left[size_t(i)] = float(std::sin(i * .03));
                right[size_t(i)] = float(std::cos(i * .04)) * .5f;
            }
            auto a = left, b = right;
            fx.process(left.data(), right.data(), 512, mod.data(), mod.data(), p);
            check(a == left && b == right,
                  "Dry/Wet zero stays sample-exact across all reverb types");
        }
        p.reverbMix = 1;
        p.reverbDecay = 20;
        p.shimmerAmount = 1;
        p.springDispersion = 1;
        p.vintageMod = 1;
        for (int block = 0; block < 500; ++block) {
            p.reverbType = block % 5;
            left = {};
            right = {};
            if (block == 0)
                left[0] = right[0] = 1;
            fx.process(left.data(), right.data(), 512, mod.data(), mod.data(), p);
            for (int i = 0; i < 512; ++i)
                check(std::isfinite(left[size_t(i)]) && std::isfinite(right[size_t(i)]) &&
                          std::abs(left[size_t(i)]) < 8 && std::abs(right[size_t(i)]) < 8,
                      "rapid type automation and maximum-tail controls remain finite during "
                      "crossfades");
        }
        fx.reset();
        left = {};
        right = {};
        fx.process(left.data(), right.data(), 512, mod.data(), mod.data(), p);
        check(std::all_of(left.begin(), left.end(), [](float v) { return v == 0; }) &&
                  std::all_of(right.begin(), right.end(), [](float v) { return v == 0; }),
              "reset removes pending tails and mode crossfades");
    }
    const std::pair<const char *, float> values[]{{"reverbtype", 4},         {"reverbdecay", 7.25f},
                                                  {"shimmeramount", .8f},    {"shimmerpitch", 7},
                                                  {"springdispersion", .9f}, {"vintagemod", .61f}};
    for (auto v : values)
        gain_test::set(processor, v.first, v.second);
    TemporaryFile preset(".logisth");
    check(processor.savePresetFile(preset.getFile()).wasOk(),
          "new reverb settings save in user presets");
    LogisthProcessor restored;
    check(restored.loadPresetFile(preset.getFile()).wasOk(),
          "new reverb settings load from user presets");
    for (auto v : values)
        check(std::abs(restored.state.getRawParameterValue(v.first)->load() - v.second) < 1e-4,
              "preset round-trip retains all model settings");
    std::unique_ptr<AudioProcessorEditor> ui(restored.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    check(editor != nullptr, "reverb editor fixture");
    auto *console = ui->getChildComponent(0);
    for (int type = 0; type < 5; ++type) {
        check(editor->snapshotPage(14 + type).isValid(), "all reverb pages render");
        for (auto item : {std::pair<const char *, bool>{"reverbdecay", type != 0},
                          {"shimmeramount", type == 4},
                          {"shimmerpitch", type == 4},
                          {"springdispersion", type == 2},
                          {"vintagemod", type == 3}}) {
            auto *field = console->findChildWithID(item.first);
            check(field && field->isVisible() == item.second,
                  "each reverb shows only its applicable controls");
        }
    }
    check(editor->validateLayout().isEmpty(),
          "new reverb controls remain reachable without overlaps");
    gain_test::set(restored, "reverbtype", 1);
    Thread::sleep(70);
    Timer::callPendingTimersSynchronously();
    check(!console->findChildWithID("shimmeramount")->isVisible(),
          "host-automated type changes refresh mode-specific control visibility");
    std::cout << "PASS reverb integration: five processor modes, preset persistence, conditional "
                 "UI and host automation\n";
}
} // namespace effects_reverb_test
