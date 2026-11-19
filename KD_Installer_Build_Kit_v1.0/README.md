# KD Core — instalator GUI

Gotowy instalator: `dist/installer/KD_Core_Setup.exe` (kopia w `C:\Users\macie\Downloads\KD_Core_Setup.exe`). Klient otrzymuje wyłącznie ten jeden plik EXE; nie uruchamia CMake, CMD ani Visual Studio.

## Co robi instalator

- standardowy kreator instalacji Windows w języku polskim,
- instalacja w `C:\Program Files\KD Core`,
- skrót w menu Start o nazwie **KD Core**,
- opcjonalny skrót na pulpicie,
- wpis w „Aplikacje i funkcje" i deinstalator,
- opcja uruchomienia programu po instalacji.

Wymagania: Windows 10/11 x64.

## Aplikacja

- proces: `KDCore.exe`,
- 215 rzeczywistych modułów diagnostycznych w 8 kategoriach,
- pomiary w czasie rzeczywistym: CPU, RAM, GPU, dyski, system,
- wykresy zasobów, raporty TXT/UTF-8 i historia skanów,
- interfejs Win32/GDI+ w palecie Graphite / Light Blue,
- ustawienia i raporty są migrowane z poprzedniej wersji programu.

## Jak powstał instalator

- aplikacja: GCC 16.2.0 (MinGW-w64, UCRT, linkowanie statyczne),
- instalator: Inno Setup 6.7.4, kompresja LZMA2/ultra64,
- renderowanie: buforowanie tła i kart, przycinanie obszarów brudnych, culling, wątki robocze.

## Testy wykonane

- kompilacja aplikacji GCC 16.2.0 i uruchomienie okna `KD Core` bez zależności od `libwinpthread-1.dll`,
- sprawdzenie 14 widoków nawigacji i otwarcia rzeczywistego raportu z historii pulpitu KD Core,
- uruchomienie modułu diagnostycznego i potwierdzenie zapisu nowego raportu w `Dokumenty\KD Core\Raporty`,
- kompilacja instalatora Inno Setup 6 jako `KD_Core_Setup.exe`, weryfikacja AppId, nazw, wersji i osadzonej ikony KD Core,
- przygotowanie ikon `KD.ico` (16–256 px) oraz PNG 64/512/1024 px i ujednolicenie znaku KD w interfejsie,
- cicha instalacja, uruchomienie i deinstalacja na tymczasowej wersji per-user: pliki, skrót menu Start i wpis w rejestrze zostały poprawnie usunięte,
- pomiary `--perf`: pełne odświeżanie 18,86 ms (53,0 FPS), małe odświeżanie 3,70 ms (270,3 FPS), bezczynność 2,2% jednego rdzenia, zestaw roboczy 36,2 MB.

Wersja produkcyjna instalatora wymaga uprawnień administratora, dlatego jej pełna instalacja nie została uruchomiona w tym środowisku; przetestowano mechanikę instalatora na tymczasowym wariancie per-user z tymi samymi plikami i ikonami.

## Uwagi

- instalator nie jest podpisany certyfikatem code signing, więc SmartScreen może pokazać ostrzeżenie,
- Inno Setup 6 jest darmowy do użytku niekomercyjnego; użycie komercyjne wymaga licencji.

## Pliki źródłowe

- `src/main.cpp`, `src/modules.hpp` — aplikacja C++17/Win32/GDI+,
- `modules_manifest.json` — opis modułów diagnostycznych,
- `resources/KD.ico` — wielorozmiarowa ikona aplikacji i instalatora (16–256 px),
- `resources/KD_Core_Icon_Master.png`, `resources/KD_Core_512.png`, `resources/KD_Core_64.png` — assets PNG dla GUI i Linuksa,
- favicon nie jest wymagany — projekt nie zawiera interfejsu WWW,
- `installer/KD_Setup.iss` — skrypt Inno Setup,
- `build_windows.bat` — developerska kompilacja MSVC (nie dla klienta),
- `.github/workflows/build-installer.yml` — alternatywne budowanie w GitHub Actions.
