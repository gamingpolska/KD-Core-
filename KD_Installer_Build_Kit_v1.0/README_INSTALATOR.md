# KD Core — instalator GUI

Gotowy instalator: `dist/installer/KD_Core_Setup.exe` (kopia w `C:\Users\macie\Downloads\KD_Core_Setup.exe`). Klient otrzymuje wyłącznie ten jeden plik EXE; nie uruchamia CMake, CMD ani Visual Studio.

SHA256 instalatora: `D729DD480D30C5F07AE19AD03552AD6B71AAD258AF1125815458EBFF1DF7C64E`  
SHA256 niezmienionego payloadu `KDCore.exe`: `F847DB76748B08108B87167DE62FF4909753F6A1A9DF6DA2E2987FE1C2032DB4`

## Interfejs

Jedno okno w stylu Liquid Glass (półprzezroczyste karty, delikatne refleksy, zaokrąglone rogi, gradient Graphite → Light Blue) z czterema zakładkami:

- **Instaluj** — nowa instalacja lub aktualizacja, wybór folderu i skrótu na pulpicie,
- **Reinstaluj** — naprawa z zachowaniem ustawień albo czysta instalacja (bez usuwania raportów),
- **Odinstaluj** — usunięcie programu z opcją zachowania ustawień,
- **Kopie zapasowe** — tworzenie i przywracanie kopii ustawień w `Dokumenty\KD Core\Kopie zapasowe`.

Okno ma własny pasek tytułu, lekkie animacje hover i wskaźnik aktywnej zakładki. Zaokrąglenie rogów realizuje DWM (Windows 11) albo DPI-skalowany region okna (starsze wersje). Standardowe strony kreatora są ukryte; instalacja, naprawa i deinstalacja korzystają z własnych ekranów postępu i podsumowania. Pola wyboru są natywnymi kontrolkami systemowymi — pozostałe powierzchnie (karty, przyciski, pola, nawigacja) mają zaokrąglone krawędzie.

## Co robi instalator

- instalacja w `C:\Program Files\KD Core`,
- skrót w menu Start o nazwie **KD Core**,
- opcjonalny skrót na pulpicie,
- wpis w „Aplikacje i funkcje" i deinstalator,
- opcja uruchomienia programu po instalacji,
- tryb cichy: `/VERYSILENT`, `/SUPPRESSMSGBOXES`, `/NORESTART`, `/LOG=...`, `/TASKS="desktopicon"`, `/CLEAN`, `/REMOVESETTINGS`.

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
- kompilacja instalatora Inno Setup 6.7.4 jako `KD_Core_Setup.exe`, weryfikacja AppId, nazw, wersji i osadzonej ikony KD Core,
- przygotowanie ikon `KD.ico` (16–256 px) oraz PNG 64/512/1024 px i ujednolicenie znaku KD w interfejsie,
- interaktywna instalacja: pliki w `C:\Program Files\KD Core`, skrót na pulpicie, kontrola integralności `KDCore.exe`, ekran podsumowania,
- zrzuty wszystkich czterech zakładek w `tests/screenshots` (również po przełączeniu zakładki),
- zestaw cichych testów funkcjonalnych: 63 z 65 sprawdzeń przeszło; dwa wymagają ponownego uruchomienia z uprawnieniami administratora,
- pomiary `--perf`: pełne odświeżanie 18,86 ms (53,0 FPS), małe odświeżanie 3,70 ms (270,3 FPS), bezczynność 2,2% jednego rdzenia, zestaw roboczy 36,2 MB.

Wersja produkcyjna instalatora wymaga uprawnień administratora. Tryb cichy obsługuje przełączniki `/CLEAN` i `/REMOVESETTINGS`; raporty w `Dokumenty\KD Core\Raporty` nie są nigdy usuwane.

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
