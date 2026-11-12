# Walidacja logisth 0.2.0

12 listopada 2026. Linux x86_64, Debian 13, GCC 14.2, kompilacja Release.

- VST3, CLAP i Standalone zbudowane pod nazwą **logisth**.
- CTest: **6/6 zaliczone**, bez testów pominiętych.
- Silnik: 44,1/48/96 kHz, strojenie, MIDI, sustain, zwalnianie nut i unison.
- Modulacja: wszystkie 27 celów i 14 źródeł; dwa cele ze znakiem ±,
  wspólny cutoff/resonance obu części, keytrack, cross-routing, 12/24 dB,
  Warm Drive, Mono Legato i synchronizacja LFO.
- Arpeggiator: 10 trybów, zdarzenia niezależne od rozmiaru bufora,
  gate, hold, pauza, transpose, swing, sustain i tempo.
- Procesor: MIDI wewnątrz bufora, zapis 248 parametrów, niepoprawny stan,
  import wcześniejszego stanu, sześć presetów i modulacja efektów.
- Efekty: wszystkie siedem sekcji, pięć typów distortion, transparentność
  płaskiego EQ i kompresora o ratio 1:1.
- Faktyczny CLAP: ładowanie modułu, stereo, 248 parametrów i render nuty MIDI.
- Faktyczny VST3: ładowanie modułu i wykrywanie klasy instrumentu.
- GUI wyrenderowane offline: widok wtyczki i cztery pełne strony.
- Próbka: 5 s stereo WAV, 48 kHz / 24-bit.

Nie wykonywano porównania odsłuchowego ani pomiarowego z oryginalnym Sylenth1.
Nie testowano w docelowym DAW ani z fizycznym urządzeniem audio. Skale DSP,
algorytmy filtrów/efektów i przyjęte różnice opisuje docs/IMPLEMENTATION.md.

Test orientacyjny silnika: 1,003 s audio przy 16 nutach, czterech oscylatorach
po 8 głosów unison wyrenderowano w ok. 0,57 s lokalnego CPU. To pomiar offline
bez master FX, nie gwarancja opóźnienia ani wydajności w DAW.

Windows/macOS: projekt i workflow przygotowane, binaria nie zostały zbudowane
w tej sesji. VST2: opcja budowania istnieje, lecz brak licencjonowanego SDK
użytkownika; binarium VST2 nie powstało. Nie konfigurowano podpisywania.

Binaria Linux wymagają glibc co najmniej 2.38.
Dla starszego systemu kompiluj lokalnie albo użyj workflow Ubuntu 22.04.
Zależności systemowe nie są pakowane z wtyczkami.

W momencie lokalnej walidacji kod nie był jeszcze wysłany na GitHub. Pliki
wydania są teraz przygotowane także w releases/logisth-0.2.0 wraz z sumami
kontrolnymi; tag v0.2.0 uruchamia workflow ich publikacji. To nie oznacza
publikacji nowej wersji środowiska chmurowego. Starsze paczki prototypu
pozostawiono lokalnie jako archiwalne.
