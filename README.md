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

[Gotowe pliki logisth 0.2.0](https://github.com/logik93/testoren/tree/master/releases/logisth-0.2.0): paczka Linux,
źródła, WAV i obrazy interfejsu. [Wydanie GitHub](https://github.com/logik93/testoren/releases/tag/v0.2.0)
jest tworzone przez workflow Publish release po wysłaniu tagu. Kompilacje Windows
oraz macOS i ich wyniki znajdziesz w [GitHub Actions](https://github.com/logik93/testoren/actions).

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
Workflow GitHub Actions przygotowuje kompilacje Linux, Windows i macOS.
Samo zapisanie workflow nie oznacza, że uruchomiono kompilacje zdalne.
Windows/macOS nie zbudowano w tej sesji; brak podpisywania i notarization.

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
