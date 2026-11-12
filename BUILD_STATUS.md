# Walidacja logisth 0.2.0

12 listopada 2026. Natywne paczki opublikowane w wydaniu:
https://github.com/logik93/testoren/releases/tag/v0.2.0

- Linux x86_64 (Ubuntu 22.04): 6/6 testów zaliczone, wymagane glibc ≥2.35.
- Windows x86_64 (Windows Server 2022): 4/4 testów zaliczone, statyczny runtime MSVC.
- macOS Universal (Intel + Apple Silicon): 4/4 testów na runnerze Intel macos-15-intel;
  wszystkie binaria zawierają x86_64 i arm64. Podpis ad-hoc, bez notarization.
- Wszystkie paczki zawierają VST3, CLAP i Standalone oraz BUILD_INFO.json.
- GitHub API potwierdziło obecność i rozmiary przesłanych archiwów. W release są
  aktualne SHA256SUMS i build-info-*.json z architekturami i rewizjami kodu.
- Kod Source, CMakeLists.txt i testy mają identyczne identyfikatory Git jak tag v0.2.0.

Weryfikacja publikacji: https://github.com/logik93/testoren/actions/runs/43285093060

Poniżej wyniki pierwotnej lokalnej walidacji na Debianie 13 / GCC 14.2.

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

Windows i macOS zostały następnie zbudowane i przetestowane w GitHub Actions,
a paczki dodano do wydania wskazanego powyżej. VST2 nadal wymaga własnego SDK
i nie jest dołączone. Windows jest bez podpisu wydawcy; macOS ma podpis ad-hoc.

Pierwotne lokalne binaria Linux w repozytorium wymagają glibc ≥2.38.
Nowsza paczka Linux w GitHub Release pochodzi z Ubuntu 22.04 i wymaga glibc ≥2.35.
Zależności systemowe nie są pakowane z wtyczkami.

W momencie lokalnej walidacji kod nie był jeszcze wysłany na GitHub. Pliki
wydania są teraz przygotowane także w releases/logisth-0.2.0 wraz z sumami
kontrolnymi; tag v0.2.0 uruchamia workflow ich publikacji. To nie oznacza
publikacji nowej wersji środowiska chmurowego. Starsze paczki prototypu
pozostawiono lokalnie jako archiwalne.
