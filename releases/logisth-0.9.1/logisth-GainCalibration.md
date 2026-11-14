# logisth output calibration — 0.9.1

The gain staging introduced in 0.6 is retained in 0.9.1. These measurements are
regenerated with the amplitude-envelope anti-click fix, new reverb controls and
clean-saw Init at Main 65%. Reverb is bypassed in the unity-sine reference;
factory presets retain their original Hall algorithm.

The reference is one full-scale 440 Hz sine, centred, at source Volume = Mix =
Main = 1, velocity 127, sustain 1, one unison voice, no modulation, filters bypassed
and all effects bypassed. Measurements use the complete plug-in processor,
including envelopes, panning, FX bypass and output gain. One second of settling
is followed by a one-second measurement. Peak and RMS use full scale = 1; RMS
is not AES17-offset.

| Sample rate | 0.5.1 peak dBFS | 0.9.1 peak dBFS | 0.9.1 RMS dBFS |
| --- | --- | --- | --- |
| 44,100 Hz | -19.4886 | 0.0000 | -3.0103 |
| 48,000 Hz | -19.4886 | 0.0000 | -3.0103 |
| 96,000 Hz | -19.4887 | 0.0000 | -3.0103 |

The old chain attenuated the engine by 0.15 (-16.48 dB) and used a -3.01 dB
centre pan law. The new source bus removes the arbitrary attenuator and applies
sqrt(2) once to establish unity per centred output channel before the effects.
There is no reciprocal post-FX booster. Main is the single post-FX output gain.
The duplicate engine Master ramp is removed, and Main smoothing starts at the
chosen value on reset. The 0.8.1 anti-click fix adds approximately 1 ms of
smoothing to each voice's amplitude envelopes, including zero Attack/Release.
The first sample of a new note is now silent; settled gain remains unity.
There is no additional startup fade on Main. See [anti-click notes](ANTICLICK.md).

Shared effects now receive a calibrated bus. A 0 dBFS sine into the compressor
at threshold -12 dB, ratio 4, attack 0.1 ms and release 100 ms peaks near -9 dBFS,
as expected from the detector. Distortion drive is intentionally retuned for
this reference. Preserving previous patch sound is not a design constraint.

At 48 kHz the exported rendered sine peaks at 0.99999994. Its measured RMS is
0.70710676. A separate generated full-scale sine supplies the independent WAV
reference. Both files are 32-bit float stereo WAV, 48 kHz, one second long.

## Factory sound reference

Four held notes: MIDI 48/55/60/64, velocity 102, six seconds, attack included,
stock effects and routing, with recalibrated factory Main trims. These are peak
measurements, not LUFS matching or guarantees for every chord or patch edit.

| Factory sound | Before, peak dBFS | After, peak dBFS at 48 kHz |
| --- | --- | --- |
| Init / saw | -21.67 | -1.24 |
| Wide supersaw | -25.19 | -6.00 |
| Sub bass | -4.60 | -13.98 |
| Slow atmosphere | -21.43 | -6.01 |
| Pulse pluck | -22.51 | -5.98 |
| Step sequence | -27.62 | -6.26 |
| Morph glass | -24.54 | -6.02 |
| Sample keys | -25.85 | -6.00 |
| Grain cloud | -30.07 | -6.84 |
| Chord pulse | -31.93 | -8.54 |

All ten sounds are additionally tested at 44.1 and 96 kHz. Nine authored sounds
retain a minimum 3 dB of reference-chord headroom. Init is now a single raw saw
at the requested Main 65% and is checked below full scale rather than against the
3 dB authored-preset trim. Its earlier measurement belongs to the previous
Init / analog sound, so that row compares different starting patches. Saved user
presets keep their old values, but gain staging and FX drive have changed. Check Main when revisiting hot patches. A dense distorted bass needs
a lower peak trim to keep its sustained RMS level sensible. The fresh-instance
Main default is 65%. Saved projects and presets retain their stored Main value.

## Regression checks

- Silent onset with a nonzero-phase carrier, unity amplitude after the short
  per-voice anti-click edge, and no additional startup fade on Main.
- Compressor reduction at the calibrated source-bus level.
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
