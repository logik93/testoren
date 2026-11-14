# logisth 0.2 — zakres zgodności z manualem

Punktem odniesienia jest dostarczony przez użytkownika `UserManual.pdf`, sekcje
3.1, 3.6–3.7 oraz 4.1–4.5. Manual opisuje architekturę i obsługę, lecz nie podaje
kodu DSP, dokładnych krzywych parametrów, nazw wszystkich przebiegów LFO ani
pełnej listy źródeł Misc. Poniżej rozdzielamy odtworzone funkcje od przyjętych
skal implementacyjnych. Nie deklarujemy zgodności próbek audio ani presetów 1:1.

## Tor sygnału i filtry

`OSC A1+A2 / OSC B1+B2 → Input Select filtrów → Filter A/B → Amp Env A/B → Mix A/B
→ Distortion → Phaser → Chorus → EQ → Reverb → Delay → Compressor → Main Volume`.

Filter Control nie dodaje trzeciego filtra. Wspólny cutoff zmienia oba lokalne
cutoff w tej samej proporcji; wspólny resonance jest dodawany do obu lokalnych
wartości. Lokalne ustawienia pozostają niezależne. Każdy filtr ma wejścia A,
B, A+B lub None, tryb LP/HP/BP/Bypass, 12/24 dB/okt. i Drive. Globalne Keytrack
wiąże cutoff z nutą; Warm Drive przełącza funkcję saturacji integratorów.
Filtry są własną implementacją SVF (24 dB to kaskada), a nie modelem układu DSP
Sylenth1. Próg rezonansu blisko maksimum umożliwia samowzbudzenie podczas nuty.

Skala cutoff:

`effectiveHz = clamp(localHz * 2^((globalSt + modulationSt + keytrackSt)/12))`.

Dla Keytrack=1 oktawa wyżej podwaja częstotliwość. Wspólne +12 st podwaja cutoff
A i B. Clamp wynosi 20 Hz–0,43 częstotliwości próbkowania.

## Modulacja

Dwa niezależne Mod Env ADSR i dwa LFO, każde z dwiema parami destination/amount.
Misc 1 i Misc 2 zawierają po dwa niezależne selektory źródła, każdy z dwoma
celami. Łącznie istnieje 16 niezależnych połączeń. Amount ma zakres −10…+10,
0 oznacza brak modulacji, dwuklik przywraca 0. Połączenia są sumowane.

27 celów (poza None), zgodnie z menu na dostarczonych zrzutach:

- Volume, Pitch, Phase, Pan: A, B lub A+B.
- Cutoff i Resonance: A, B lub A+B.
- Phaser Frequency; Mix A, B lub A+B.
- LFO1 Rate/Gain, LFO2 Rate/Gain; Distortion Amount.

Źródła Misc: Velocity, Keytrack, Aftertouch, Mod Wheel, Pitch Wheel,
Amp Env A/B, Mod Env 1/2, LFO 1/2, Step Velocity, Random i Random bipolar.
Aftertouch obsługuje zarówno channel pressure, jak i poly pressure; poly pressure
ma pierwszeństwo dla danej nuty. Random jest losowany osobno przy note-on.

Przy Amount=10 i źródle=1 przyjęto następujące pełne zakresy:

| Cel | Dodawana zmiana |
| --- | --- |
| Cutoff | +120 półtonów |
| Pitch | +48 półtonów |
| Phase | +1 okres |
| Pan | +1 (środek → prawo) |
| Volume, Mix, Resonance, Gain, Distortion | +1 w skali liniowej |
| LFO Rate | mnożnik 16 |
| Phaser Frequency | mnożnik 1024 |

Ujemny amount odwraca zmianę. Te skale są jawnie przyjętymi zakresami logisth;
manual nie określa dokładnych współczynników oryginału. Volume i Mix są
**dodawane**, więc Step Velocity → Mix A+B może otworzyć warstwy ustawione na 0.
Cele LFO Rate/Gain wykorzystują modulację z poprzedniej próbki, aby sprzężenia
LFO nie tworzyły pętli algebraicznej. Efekty są globalne: ich modulacja pochodzi
z najnowszego aktywnego głosu. Zakończenie tego głosu może zmienić źródło FX.

LFO ma 10 przebiegów, Rate, Gain, Offset i Free. Free nie retriggeruje fazy ani
nie używa zegara hosta. Nowy głos Free przejmuje bieżącą fazę zegara free-running.
Pozostałe LFO startują od początku przy nucie i przy globalnym Sync wykorzystują
wybrany podział rytmiczny. Przy modulacji Rate fazy poszczególnych głosów Free
mogą się rozchodzić. Gałki Hz/czasu są nieaktywne podczas synchronizacji;
wtedy używane są selektory podziału. Tempo awaryjne bez hosta: 120 BPM.

## Arpeggiator

Dziesięć trybów, pięć trybów velocity, 1–4 oktawy, Wrap, Time/division, Gate,
Swing oraz 16 kroków z Transpose, Velocity i Hold. Velocity=0 tworzy pauzę.
Step Seq gra jeden dźwięk, Step Chord przetwarza akord. Hold wiąże kolejne
kroki; zmiana wysokości może wywołać portamento w Mono Legato. Step Velocity
jest osobnym źródłem modulacji i spada do 0 po zakończeniu gate.

Implementacja Step+Key / Step+Hold mnoży obie znormalizowane wartości velocity;
manual mówi o ich mieszaniu, ale nie definiuje dokładnej funkcji. Up/Down oraz
Down/Up pomijają powtórzenie skrajnych nut; warianty „repeat” je powtarzają.
Ordered zachowuje kolejność naciskania klawiszy. Arp obsługuje sustain i zatrzymuje
nuty po stop/seek hosta oraz zmianie enable. Czas zdarzeń nie zależy od wielkości
bufora hosta. Włączenie arp czyści aktualne nuty; należy ponownie nacisnąć klawisze.

## Efekty i gra

Wszystkie siedem sekcji ma bypass i własne parametry opisane w manualu:

- Distortion: Overdrive, Foldback, Clip, Decimate, Bitcrush, Amount i Dry/Wet.
- Phaser: sześć stopni stereo, Center, Spread, LR Offset, Width, LFO,
  Feedback i Dry/Wet; Center reaguje na macierz modulacji.
- Chorus: Delay, Depth, Rate, Feedback, Dual, Width i Dry/Wet.
- EQ (0.9.2): poprawione półki Bass/Treble w trybie Legacy oraz 10-pasmowy Graphic; opcjonalny Auto Gain, krzywa i Output.
- Reverb: Size, Damp, Predelay, Width i Dry/Wet (algorytm JUCE Reverb).
- Delay: niezależny L/R, Sync, Feedback, PingPong, Spread, Width, filtry,
  Smear i Dry/Wet. Maksymalna pamięć echa to 16 s na kanał; dłuższe wartości
  wynikające z bardzo wolnego tempa są ograniczane do 16 s.
- Compressor: stereo linked, Ratio, Threshold, Attack i Release.

Main Volume działa po efektach. Sygnał wyjściowy nie ma twardego limitera;
przy wysokich poziomach może przekroczyć 0 dBFS, zgodnie z opisem manualu.
Przełączniki bypass zatrzymują przetwarzanie danej sekcji; jej pamięć może
pozostać zamrożona do ponownego włączenia. Parametry ciągłe master/cutoff i
wybrane miksy są wygładzane; zmiany routingu, czasu delay i przebiegów mogą klikać.

Oscylatory oferują 0–8 voices, octave/note/fine, phase/retrigger, stereo, pan
oraz invert. Mono Legato ma priorytet ostatniej nuty i powrót do poprzedniej
trzymanej nuty. Normal Portamento działa przy nakładających się klawiszach,
Slide także między osobnymi nutami. Dostępne są ekranowe pitch/mod wheel,
Bend Range, Solo A/B i polifonia do 16 nut. Przełącznik A/B wybiera edytowaną
warstwę wewnątrz jednego okna; zmiana widoku nie zmienia parametrów ani brzmienia.
THEME wybiera jasny lub ciemny motyw zapisany w stanie danej instancji.

Nie ma importu banków/presetów Sylenth1 ani jego oryginalnych grafik, menu
zarządzania bankami i MIDI Learn. Własny zapis stanu zawiera wszystkie 248
parametrów, także tablicę kroków i routowanie. Nowa tożsamość wtyczki wymaga
wybrania logisth w DAW. Import wcześniejszego stanu prototypu mapuje ustawienia
filtrów i stare modulacje na nowe połączenia, ale nie gwarantuje identycznego
brzmienia po zmianie algorytmów.
