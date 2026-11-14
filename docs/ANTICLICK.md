# Zero-time ADSR click fix — 0.8.1

## Cause and correction

Previously, zero Attack still occupied one audio sample at full envelope gain.
Zero Decay then reached Sustain on the next sample. With A = D = S = R = 0,
this emitted a full-amplitude impulse even though the note should be silent.
A centred sine starting at 90 degrees reproduced a 0 dBFS single-sample impulse
per channel, with the calibrated source/Mix gains at unity.

Zero-duration stages now consume no samples. The envelope enters its requested
Sustain directly. A = D = S = 0 produces exact digital silence on both channels.

A separate problem affected audible notes with Sustain above zero: zero
Attack/Release abruptly gated the waveform at its current phase. Part A/B and
enabled per-oscillator amplitude envelopes now pass through a short, causal
triangular smoothing window (approximately 1 ms). The first sample is silent,
and a zero Release preserves the carrier at the note-off boundary before fading
to exact zero within 1.1 ms at the tested rates. This window also smooths nonzero
amplitude-envelope segments and adds a short bounded tail. The drawn ADSR curve
continues to show the requested parameter times.

The smoothing has unity settled gain and no overshoot. It processes envelope
gain rather than filtering the audio waveform. It uses fixed storage and
constant work per sample, with no allocation or lock in the audio callback.
Modulation envelopes keep instantaneous zero-time transitions and their existing
nonzero attack rates. Main calibration remains independent of this per-voice edge.

## Reproduction and regression checks

Set one oscillator to sine, Retrig on, Phase 90 degrees; bypass filters, effects,
arp and modulation. Set source Volume, part Mix and Main to 100%, with one unison
voice, centred pan and MIDI velocity 127. Play MIDI 69 with all ADSR values at zero.
The old output was one sample at amplitude 1; the corrected output is zero.
Repeat with Sustain 100%: the note should sound at the calibrated level, with
short smooth edges at note-on and note-off even when Attack/Release are zero.

Automated checks at 44.1, 48 and 96 kHz cover:

- 1,152 silent-output fixtures: all four slots, all eight classic waveforms,
  wavetable, sampler and both granular models; phase 0/90/217 degrees and random
  phase; zero ADSR on the part or the enabled oscillator envelope.
- 192 released-versus-held carrier comparisons across all classic waves and
  slots, with oscillator envelopes enabled and disabled. Carrier noise and
  saw/pulse edges are compared to the same running reference rather than
  incorrectly counting their natural discontinuities as gate clicks.
- Silent onset, monotonic bounded gain edges, no overshoot, exact steady sustain,
  bounded release completion and one-sample/short-note tails.
- Full-processor MIDI events at samples 37 and 301, all eight waves and three
  rates, compared sample for sample across host buffers of 1/31/257/512 samples.
- Full-processor sine calibration: 0 dBFS peak and -3.0103 dBFS RMS per channel
  after settling, plus the existing gain, summing, effects and factory-headroom checks.

The envelope/engine tests run in `engine_tests`; MIDI integration and calibration
run in `processor_tests`. Run the full configured test suite with:

```
ctest --test-dir build --output-on-failure
```

Local Linux validation passed all eight suites, including the compiled VST3 and
CLAP binary smoke tests. Settled sine measurements remain 0 dBFS peak and
-3.0103 dBFS RMS per channel at all three tested rates.
