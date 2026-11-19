# Analiza źródeł v7.1–v7.3

> Uwaga historyczna: nazwy „Kombajn Diagnostyczny” poniżej opisują wyłącznie archiwa źródłowe i oryginalny produkt analizowany przed portem. Obecną nazwą produktu jest **KD Core**.

## Pliki sprawdzone

- `Kombajn_Diagnostyczny_v7_1_fix_math(1).zip`
- `Kombajn_Diagnostyczny_v7_2_antialias.zip`
- `Kombajn_Diagnostyczny_v7_3_rounded_ui.zip`

Najświeższe archiwum v7.3 zawierało `Kombajn_Diagnostyczny_v7.py` oraz skrypt budujący EXE przez PyInstaller. Sprawdzono, że tabele modułów w v7.1, v7.2 i v7.3 są identyczne: każda ma 120 nazw, 120 wpisów poleceń i 9 kategorii. Nazwy, kategorie i polecenia portu odczytano z v7.3.

## Zakres źródłowego programu

- 120 modułów w 9 kategoriach.
- Wyszukiwanie po nazwie, kategorii i numerze.
- Wybór modułu, uruchomienie przyciskiem lub dwuklikiem.
- Panel wyników, czyszczenie i eksport raportu TXT.
- Statystyki CPU, RAM, dysku systemowego i informacji o Windows.
- Potwierdzenia dla wybranych operacji zmieniających stan systemu.

Moduły 1–120 i ich polecenia znajdują się w `src/modules.hpp`; wersja czytelna do edycji jest też w `modules_manifest.json`.

## Znaleziona usterka v7.3

W archiwum v7.3 linia importów zawiera `Image`, `ImageDraw` i `ImageTk`, ale metoda `make_app_icon()` korzysta również z `ImageFont`. Brak importu `ImageFont` powoduje zgłoszony wcześniej `NameError`. W tym samym archiwum `ImageTk` jest importowane; komunikat o braku `ImageTk` wskazuje, że uruchamiany lokalnie plik różnił się od pliku w przesłanym ZIP-ie albo linia importu została zmieniona podczas ręcznej poprawki.

Wersja C++ nie korzysta z Pillow, Tkintera ani importów Pythona.

## Charakter portu C++

Okno, interfejs, rysowanie, statystyki, filtrowanie, obsługa zdarzeń, eksport i uruchamianie procesów są napisane w C++17 z użyciem Win32 i GDI+. Polecenia diagnostyczne pozostają poleceniami Windows/PowerShell z v7.3, uruchamianymi przez `cmd.exe`, ponieważ tak działał oryginalny zestaw 120 modułów. Dzięki temu zachowano istniejący zakres i zachowanie narzędzi zamiast zastępować je niezweryfikowanymi implementacjami.

Moduł 37 „Foldery systemowe” nie miał przypisanego polecenia w źródle v7.3. W porcie zachowano komunikat o braku polecenia.
