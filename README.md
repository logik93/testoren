# logisth 0.2

Dwuwarstwowy syntezator subtraktywny w formatach VST3, CLAP i Standalone.
Rozbudowana wersja pierwotnego prototypu: wspólny Filter Control, filtry A/B,
pełny routing modulacji z pokazanych menu, arpeggiator, siedem sekcji efektów,
Mono Legato i portamento. Własny kod i interfejs, bez powiązania z LennarDigital.

- 16 nut polifonii, cztery oscylatory, do 8 głosów unison na oscylator.
- 8 przebiegów OSC, octave/note/fine, phase/retrigger, invert, stereo i pan.
- Filter A/B: LP/HP/BP/Bypass, 12/24 dB, Drive i Input Select A/B/A+B/None.
- Filter Control: wspólne cutoff/resonance, Keytrack i Warm Drive.
- Dwa Amp Env, dwa Mod Env, dwa LFO po 10 przebiegów i cztery źródła Misc.
- 16 połączeń modulacji z amount ±10, 27 celów i 14 wybieralnych źródeł.
- Arpeggiator: 10 trybów, 5 trybów velocity, 16 kroków, hold, transpose,
  velocity/rest, swing, gate, wrap i oktawy.
- Distortion, Phaser, Chorus, EQ, Reverb, Delay i Compressor z bypass.
- Host Sync, Solo A/B, pitch/mod wheel, Mono Legato i dwa tryby portamento.
- 248 automatyzowalnych parametrów, zapis stanu DAW, sześć presetów fabrycznych.

Strony interfejsu: **Synthesis**, **Modulation**, **Arpeggiator**, **Effects**.
Każdą stronę można przewinąć. Przy Host Sync wybierz podział rytmiczny zamiast
nieaktywnej gałki Hz/sekund. LFO Free ignoruje Sync. Dwuklik amount przywraca 0.
Main Volume znajduje się po efektach; wyjście może przekraczać 0 dBFS.

Architektura została odniesiona do dostarczonego `UserManual.pdf`. Manual nie
opisuje szczegółów DSP ani dokładnych krzywych parametrów, więc nie deklarujemy
zgodności brzmienia 1:1. [Zakres, skale i różnice](docs/IMPLEMENTATION.md).

## Pobieranie

[Wydanie logisth 0.2.0](https://github.com/logik93/testoren/releases/tag/v0.2.0)
zawiera gotowe paczki Linux x64, Windows x64 oraz macOS Universal (Intel i Apple
Silicon). Każda zawiera VST3, CLAP i Standalone. Wszystkie kompilacje przeszły
testy: Linux 6/6, Windows 4/4, macOS 4/4 na runnerze Intel. macOS jest podpisane
ad-hoc i wymaga macOS 11+; Linux wymaga glibc ≥2.35. Dołączono źródła, WAV,
obrazy interfejsu, metadane kompilacji i wspólne SHA256SUMS.

[Weryfikacja GitHub Actions](https://github.com/logik93/testoren/actions/runs/43285093060).
Starsze lokalne paczki w katalogu releases/ są archiwalne; pobieraj aktualne
binaria z GitHub Release.

## Budowanie

Wymagane: Git, CMake ≥3.22 i kompilator C++17 (GCC/Clang, Visual Studio 2022 lub
Xcode). JUCE 8.0.6 i clap-juce-extensions są przypięte do commitów w CMake.
Używaj istniejącego checkoutu; nowy worktree nie jest potrzebny.

Linux, Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake git libasound2-dev libx11-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

Windows, terminal deweloperski Visual Studio 2022:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

macOS, Xcode command line tools i CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

Wyniki: `build/Logisth_artefacts/Release/{VST3,CLAP,Standalone}`. Nazwa produktu
we wszystkich formatach: **logisth**, CLAP ID/bundle ID: `com.testoren.logisth`.
Workflow Native builds and release kompiluje Linux x64, Windows x64 i macOS
Universal (Intel + Apple Silicon), uruchamia testy, sprawdza architektury binariów
i pakuje VST3/CLAP/Standalone. Po sukcesie wszystkich trzech kompilacji dodaje
paczki oraz wspólne SHA256SUMS do wydania v0.2.0. BUILD_INFO.json w paczce zapisuje
rewizję kodu, architektury i wyniki testów. Workflow sprawdza zgodność kodu z tagiem
przed publikacją. PR-y wykonują testy bez publikowania. Brak podpisu wydawcy
Windows i notarization; macOS ma podpis ad-hoc. Wynik CI jest zapisywany także
na gałęzi ci-release-v0.2.0 w status.json.

VST2 jest opcjonalne i wymaga własnego licencjonowanego SDK, którego użytkownik
nie posiada. Bez niego nie powstaje binarium VST2.

```sh
cmake -S . -B build-vst2 -DCMAKE_BUILD_TYPE=Release \
  -DLOGISTH_VST2=ON -DVST2_SDK_PATH=/absolute/path/to/vstsdk2.4
cmake --build build-vst2 --config Release --parallel 3
```

SDK musi zawierać `pluginterfaces/vst2.x/aeffect.h` i `aeffectx.h`. VST2 nie
zostało zweryfikowane. Nie dołączamy ani nie pobieramy archiwalnego SDK.

## Instalacja wtyczek

Skopiuj cały bundle `.vst3` i plik/bundle `.clap` do ścieżki skanowanej przez DAW.

| System | VST3 użytkownika | CLAP użytkownika |
| --- | --- | --- |
| Linux | `~/.vst3/` | `~/.clap/` |
| Windows | `%LOCALAPPDATA%\Programs\Common\VST3\` | `%LOCALAPPDATA%\Programs\Common\CLAP\` |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` | `~/Library/Audio/Plug-Ins/CLAP/` |

Przeskanuj wtyczki w DAW i wstaw **logisth** na ścieżkę instrumentu MIDI.
Standalone wymaga ustawienia urządzenia audio/MIDI. Zmiana nazwy i identyfikatora
oznacza osobny instrument w hostach; dotychczasowe instancje prototypu nie są
automatycznie podmieniane. Starsze paczki w `dist` pozostawiono jako archiwalne.

## Testy i materiały podglądowe

```sh
ctest --test-dir build -C Release --output-on-failure
./build/processor_tests "$(pwd)/dist/logisth-0.2.0"
# Windows: .\build\Release\processor_tests.exe "C:\absolute\output-directory"
```

Testy obejmują generowanie audio przy 44,1/48/96 kHz, wszystkie źródła i cele
modulacji, filtry wspólne i niezależne, cross-routing, mono, tryby i timing arp,
efekty, zapis wszystkich parametrów i import starego stanu. Testy binariów Linux
ładują rzeczywiste CLAP i VST3. CLAP renderuje nutę; VST3 sprawdza wykrycie klasy
instrumentu, nie zastępuje testu w DAW. `processor_tests` z katalogiem wyjściowym
generuje WAV oraz obrazy interfejsu i czterech pełnych stron.

Silnik/modulacja/arp bez JUCE i GUI:

```sh
cmake -S . -B build-engine -DLOGISTH_PLUGINS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine
ctest --test-dir build-engine --output-on-failure
```

Aktualne wyniki i ograniczenia binariów: [BUILD_STATUS.md](BUILD_STATUS.md).

## Licencje zależności

JUCE: AGPLv3 lub licencja komercyjna JUCE. Przed dystrybucją wybierz model zgodny
z tymi warunkami; zamknięty produkt może wymagać licencji komercyjnej. Szczegóły:
`.deps/JUCE/LICENSE.md` lub oficjalne repozytorium JUCE. clap-juce-extensions oraz
CLAP: MIT. SDK VST2 ma osobne warunki Steinberga. Manual i oryginalne zasoby
Sylenth1 nie są dołączane do paczki źródeł ani binariów.
