# logisth output calibration — 0.5.2

The reference is one full-scale 440 Hz sine, centred, at source Volume = Mix =
Main = 1, velocity 127, sustain 1, one unison voice, no modulation, filters bypassed
and all effects bypassed. Measurements use the complete plug-in processor,
including envelopes, panning, FX bypass and output gain. One second of settling
is followed by a one-second measurement. Peak and RMS use full scale = 1; RMS
is not AES17-offset.

| Sample rate | 0.5.1 peak dBFS | 0.5.2 peak dBFS | 0.5.2 RMS dBFS |
| --- | --- | --- | --- |
| 44,100 Hz | -19.4886 | -0.0001 | -3.0104 |
| 48,000 Hz | -19.4886 | -0.0001 | -3.0104 |
| 96,000 Hz | -19.4887 | -0.0002 | -3.0105 |

The old chain attenuated the engine by 0.15 (-16.48 dB) and used a -3.01 dB
centre pan law. A final scalar sqrt(2)/0.15 (+19.4885 dB) restores unity per
centred output channel. Its position after shared effects retains their input
level, distortion colour and compressor operation. The engine's original
headroom factor is retained and explicitly named. Float smoother precision
accounts for the approximately 0.0001–0.0002 dB remaining deviation.

At 48 kHz the exported rendered sine peaks at 0.9999856. Its measured RMS is
0.7070967. A separate generated full-scale sine supplies the independent WAV
reference. Both files are 32-bit float stereo WAV, 48 kHz, one second long.

## Factory sound reference

Four held notes: MIDI 48/55/60/64, velocity 102, six seconds, attack included,
stock effects and routing. Only the post-FX Main trim is changed. These are peak
measurements, not LUFS matching or guarantees for every chord or patch edit.

| Factory sound | Before, peak dBFS | After, peak dBFS at 48 kHz |
| --- | --- | --- |
| Init / analog | -21.67 | -5.97 |
| Wide supersaw | -25.19 | -6.00 |
| Sub bass | -4.60 | -6.01 |
| Slow atmosphere | -21.43 | -6.01 |
| Pulse pluck | -22.51 | -6.00 |
| Step sequence | -27.62 | -6.01 |
| Morph glass | -24.54 | -6.01 |
| Sample keys | -25.85 | -6.01 |
| Grain cloud | -30.07 | -6.84 |
| Chord pulse | -31.93 | -8.70 |

All ten sounds are additionally tested at 44.1 and 96 kHz with a minimum 3 dB
reference-chord headroom requirement. Saved user presets keep their old values;
at identical settings the output is +19.49 dB louder. Main should be lowered
when revisiting hot patches. The fresh-instance Main default is 42%.

## Regression checks

- Full processor sine peak and RMS at 44.1, 48 and 96 kHz; stereo channel equality.
- Half source/Mix/Main gain: -6.0206 dB each.
- MIDI velocity follows its actual seven-bit value (a float 0.5 becomes 64/127).
- Part B has the same unity level as Part A.
- Hard pan: +3.0103 dB in the active channel and silence in the other.
- Two coherent oscillators: +6.0206 dB; eight coherent unison voices: +9.0309 dB.
- Unit-amplitude wavetable, sample and Granulizer input each reach unity peak.
- Factory reference chords retain headroom at all three sample rates.
- Existing scope tests compare its final sum against actual L+R output samples.

Coherent summing tests deliberately exceed 0 dBFS and verify float headroom,
rather than adding a limiter that would alter dynamics. Reduce Main/Mix when
stacking oscillators or polyphony. The mixer shows dBFS and red overload values;
the L+R scope is a sum, so a centred 0 dBFS-per-channel sine sums to +6.02 dBFS.

The checks run inside the existing processor test suite. To regenerate measurements
and the calibration preset/audio, run:

```
build/processor_tests --gain-check /absolute/output/folder
```

The CSV files in docs/calibration record the measured before/after readings.
The release ZIP contains both WAVs, the calibration preset, these CSVs and this
report. For a DAW check, load the preset, play MIDI note 69 at velocity 127, set
the DAW track fader to unity, and bypass track effects.
