#pragma once
#include "GainCalibration.h"
#include "PluginEditor.h"
namespace processor_phase_test {
using namespace juce;
inline void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void configure(LogisthProcessor &p, int slot, int mode, bool retrig, float phase) {
    gain_test::cleanSine(p);
    gain_test::set(p, "o1level", 0);
    auto id = "o" + String(slot + 1);
    gain_test::set(p, id + "level", .2f);
    gain_test::set(p, id + "engine", float(mode));
    gain_test::set(p, id + "retrig", retrig ? 1.f : 0.f);
    gain_test::set(p, id + "phase", phase);
    p.prepareToPlay(48000, 512);
}
inline AudioBuffer<float> render(LogisthProcessor &p, int length, bool noteOn = false) {
    AudioBuffer<float> result(2, length), block(2, 512);
    MidiBuffer midi;
    if (noteOn)
        midi.addEvent(MidiMessage::noteOn(1, 69, 1.f), 0);
    for (int offset = 0; offset < length; offset += 512) {
        int n = std::min(512, length - offset);
        block.setSize(2, n, false, false, true);
        p.processBlock(block, midi);
        midi.clear();
        for (int ch = 0; ch < 2; ++ch)
            result.copyFrom(ch, offset, block, ch, 0, n);
    }
    return result;
}
inline void run() {
    for (int slot = 0; slot < 4; ++slot)
        for (int mode = 0; mode < 2; ++mode)
            for (bool retrig : {false, true}) {
                LogisthProcessor live, goal, base;
                configure(live, slot, mode, retrig, 0);
                configure(goal, slot, mode, retrig, 90);
                configure(base, slot, mode, retrig, 0);
                render(live, 317, true);
                render(goal, 317, true);
                render(base, 317, true);
                gain_test::set(live, "o" + String(slot + 1) + "phase", 90);
                auto actual = render(live, 4800), target = render(goal, 4800),
                     unchanged = render(base, 4800);
                double difference = 0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 4800; ++i) {
                        check(std::isfinite(actual.getSample(ch, i)),
                              "host phase automation stays finite");
                        difference +=
                            std::abs(actual.getSample(ch, i) - unchanged.getSample(ch, i));
                        if (i >= 4672)
                            check(std::abs(actual.getSample(ch, i) - target.getSample(ch, i)) <
                                      1e-5,
                                  "host automation applies PHASE to held notes in every "
                                  "slot/mode/retrigger policy");
                    }
                check(difference > 1,
                      "host PHASE parameter produces an audible change while RETRIG is off or on");
            }
    LogisthProcessor p;
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    check(editor != nullptr, "phase GUI fixture");
    for (int slot = 0; slot < 4; ++slot)
        for (int mode = 0; mode < 2; ++mode) {
            configure(p, slot, mode, false, 0);
            editor->snapshotPage(slot < 2 ? 0 : 1);
            auto *field =
                ui->getChildComponent(0)->findChildWithID("o" + String(slot + 1) + "phase");
            check(field != nullptr, "phase GUI parameter component exists");
            Slider *knob = nullptr;
            for (auto *child : field->getChildren())
                if (auto *slider = dynamic_cast<Slider *>(child); slider && slider->isVisible())
                    knob = slider;
            check(field->isVisible() && knob && knob->isEnabled(),
                  "PHASE knob is visible and enabled with RETRIG off in A1/A2/B1/B2 "
                  "Classic/Wavetable");
            Thread::sleep(70);
            Timer::callPendingTimersSynchronously();
            check(knob->isEnabled(), "GUI timer cannot re-disable PHASE after a refresh");
            knob->setValue(137, sendNotificationSync);
            check(std::abs(p.state.getRawParameterValue("o" + String(slot + 1) + "phase")->load() -
                           137) < 1e-4,
                  "enabled PHASE knob updates its real host parameter");
        }
    std::cout << "PASS processor PHASE: 16 held-note automation cases, all four player knobs "
                 "enabled without RETRIG, GUI refresh and bindings\n";
}
} // namespace processor_phase_test
