Pierwsze wydanie **logisth**: dwuwarstwowy syntezator z czterema oscylatorami,
unison i polifonią do 16 nut.

- Filtry A/B oraz wspólny Filter Control, Keytrack i Warm Drive.
- Dwa Mod Env, dwa LFO, cztery źródła Misc: 16 połączeń, 27 celów modulacji.
- Arpeggiator z 16 krokami, siedem efektów, Mono Legato i portamento.
- 248 parametrów automatyzacji, sześć presetów i zapis stanu.

Załączniki zawierają sprawdzone binaria **Linux x86_64**: VST3, CLAP i Standalone,
kod źródłowy, pięciosekundową próbkę WAV, obrazy interfejsu i sumy SHA-256.
Binaria Linux wymagają glibc ≥2.38. Testy lokalne: **6/6 zaliczone**.

Workflow Build and test przygotowuje osobne kompilacje Linux, Windows i macOS;
ich wyniki są dostępne jako artefakty ukończonych zadań GitHub Actions.
VST2 wymaga własnego SDK i nie jest dołączone.

Architektura jest oparta na funkcjach opisanych w manualu Sylenth1, z własnym
kodem i interfejsem. Nie deklarujemy zgodności brzmienia lub presetów 1:1.
Skale DSP i ograniczenia opisano w docs/IMPLEMENTATION.md. Przed dystrybucją
pochodnych produktów należy uwzględnić warunki JUCE i pozostałych zależności.
