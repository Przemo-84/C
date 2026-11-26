# Controls – własne kontrolki Win32 w C++

Zestaw pięciu kontrolek do aplikacji okienkowych Windows. Powstały na potrzeby aplikacji autora, w której posłużyły do zbudowania interfejsu z własnym menu, przyciskami i polami wyboru. Każda kontrolka ma własną klasę C++ i procedurę okna. Za rysowanie odpowiada Windows GDI.

## Kontrolki

| Klasa | Zastosowanie | Co można ustawić |
| --- | --- | --- |
| `Sys_Button` | Przycisk na własnym pasku tytułu okna | Symbol akcji, rozmiar, kolor symbolu i tło w stanie zwykłym oraz po najechaniu myszą |
| `MenuButton` | Przycisk otwierający menu poleceń | Podpisy i identyfikatory pozycji, ich dostępność, kolory oraz położenie rozwijanego menu |
| `ButtonControl` | Przycisk graficzny, np. ikona na pasku narzędzi | Dwie bitmapy – zwykła i wyświetlana po najechaniu myszą |
| `CheckBox` | Pole wyboru z podpisem | Stan początkowy, kolor zaznaczenia oraz kolory tekstu i tła |
| `StaticText` | Etykieta, np. opis pola lub komunikat o stanie aplikacji | Treść, pozycja, krój i rozmiar pisma, kolor tekstu i pędzel tła |

## Dodanie do projektu w Visual Studio

1. Otwórz projekt aplikacji okienkowej C++ korzystającej z Win32 API. Konfiguracja sprawdzona dla tego zestawu to **Visual Studio 2022, Win32 (x86), Unicode**.
2. Skopiuj [Controls.h](Controls.h) i [Controls.cpp](Controls.cpp) do katalogu projektu. W Eksploratorze rozwiązań wybierz **Dodaj → Istniejący element** i dodaj oba pliki.
3. Projekt powinien linkować z `User32.lib` i `Gdi32.lib`. Jeśli szablon wymaga `pch.h`, możesz wyłączyć prekompilowane nagłówki we właściwościach samego pliku `Controls.cpp`.
4. W pliku obsługującym okno dodaj `#include "Controls.h"`. Kontrolki możesz tworzyć w obsłudze `WM_CREATE`, przekazując uchwyt okna rodzica (`HWND`) i instancję aplikacji (`HINSTANCE`).

Pozycje i rozmiary kontrolek podaje się w pikselach. Interfejs klas przyjmuje napisy jako `char*`, dlatego przykłady używają tablic, np. `char podpis[] = "Plik";`.

## Przykład – pole wyboru i reakcja okna

W tym przykładzie kliknięcie pola „Widok rozszerzony” zmienia tytuł okna. Kontrolka wysyła swój uchwyt w `wParam`, a nowy stan zaznaczenia w `lParam`. Nadany przy tworzeniu identyfikator pozwala rodzicowi odnaleźć właściwy uchwyt przez `GetDlgItem`.

```cpp
#include "Controls.h"

constexpr int IdSzczegoly = 101;

LRESULT CALLBACK OknoProc(HWND okno, UINT komunikat, WPARAM wParam, LPARAM lParam)
{
    switch (komunikat)
    {
        case WM_CREATE:
        {
            auto* dane = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* pole = new CheckBox(okno, dane->hInstance);
            char podpis[] = "Widok rozszerzony";

            pole->CreateControl(
                okno, 16, 16, 180, 28, podpis,
                RGB(245, 245, 245), RGB(225, 235, 250), // tło i tło po najechaniu
                RGB(25, 90, 160),                      // znak zaznaczenia
                RGB(30, 30, 30), RGB(15, 65, 120),     // tekst i tekst po najechaniu
                false);

            if (!pole->MainWND)
            {
                delete pole;
                return -1;
            }
            SetWindowLongPtrW(pole->MainWND, GWLP_ID, IdSzczegoly);
            return 0;
        }
        case WM_COMMAND:
        {
            HWND pole = GetDlgItem(okno, IdSzczegoly);
            if (pole && wParam == reinterpret_cast<WPARAM>(pole))
            {
                SetWindowTextW(okno, lParam ? L"Szczegóły włączone" : L"Szczegóły wyłączone");
                return 0;
            }
            break;
        }
    }
    return DefWindowProcW(okno, komunikat, wParam, lParam);
}
```

Jeżeli zachowujesz wskaźnik do obiektu, możesz odczytać stan przez `Check_isChecked()` lub ustawić go przez `Check_check(true)`. Zmiana programowa odświeża pole. Powiadomienie `WM_COMMAND` powstaje po kliknięciu przez użytkownika.

## Przykład – menu „Plik”

Funkcję `DodajMenu` możesz wywołać podczas tworzenia okna. Polecenie „Zapisz” jest od razu dostępne. Menu wyświetla też pozycję „Eksportuj”, którą aplikacja może włączyć później.

```cpp
constexpr int IdZapisz = 1001;
constexpr int IdEksportuj = 1002;

MenuButton* DodajMenu(HWND okno, HINSTANCE instancja)
{
    char tytul[] = "Plik";
    char zapisz[] = "Zapisz";
    char eksportuj[] = "Eksportuj";
    auto* menu = new MenuButton(okno, instancja);

    if (menu->CreateControl(okno, 8, 8, tytul,
                            RGB(30, 30, 30), RGB(0, 80, 160), RGB(145, 145, 145)) != 0)
    {
        delete menu;
        return nullptr;
    }
    menu->AddItem(IdZapisz, zapisz, TRUE);
    menu->AddItem(IdEksportuj, eksportuj, FALSE);
    return menu;
}
```

Po wybraniu „Zapisz” procedura rodzica otrzymuje `WM_COMMAND` z wartością `IdZapisz` w `wParam`. Metody modyfikujące pozycje operują na **indeksie liczonym od zera**. Wywołanie `menu->SetMenuitemEnable(1, TRUE)` włącza więc „Eksportuj”. Podpis pozycji można zmienić przez `SetMenuitemText`. Parametr `Item_Mode` w `CreateControl` pozwala otworzyć menu nad lub pod przyciskiem, z wyrównaniem do lewej lub prawej krawędzi. Domyślne ustawienie to `bott_left`.

## Komunikaty wysyłane do okna rodzica

| Kontrolka i zdarzenie | Komunikat lub akcja | `wParam` | `lParam` |
| --- | --- | --- | --- |
| `MenuButton` – wybór aktywnej pozycji | `WM_COMMAND` przez `PostMessage` | Identyfikator przekazany do `AddItem` | `0` |
| `ButtonControl` – wciśnięcie lewego przycisku myszy | `WM_COMMAND` przez `SendMessage` | Uchwyt `MainWND` przycisku | `0` |
| `CheckBox` – zwolnienie lewego przycisku myszy | `WM_COMMAND` przez `SendMessage` | Uchwyt `MainWND` pola | `1` po zaznaczeniu, `0` po odznaczeniu |
| `Sys_Button` – zamknięcie | `WM_CLOSE` przez `SendMessage` | `0` | `0` |

W trybie `minimize` kliknięcie przycisku wywołuje `ShowWindow` z flagą `SW_MINIMIZE` dla okna rodzica. W trybie `maximize` używana jest flaga `SW_MAXIMIZE`.

## Czas życia i zasoby

- Utwórz obiekt kontrolki za pomocą `new`. Po utworzeniu jej okna procedura obsługi `WM_DESTROY` sama usuwa obiekt C++. Zachowany wskaźnik jest ważny tylko do chwili zniszczenia okna kontrolki. Jeśli chcesz ją usunąć wcześniej, wywołaj `DestroyWindow` na jej `MainWND`.
- `CheckBox::CreateControl` i `ButtonControl::CreateControl` zwracają `0` również wtedy, gdy utworzenie okna się nie powiedzie. Wynik sprawdzisz przez `MainWND`, jak w przykładzie z polem wyboru.
- Do `ButtonControl` przekaż dwie osobne bitmapy w rozmiarze przycisku. Kontrolka zwalnia ich uchwyty przy niszczeniu. Podobnie `Sys_Button`, `MenuButton` i `StaticText` zwalniają przekazane im pędzle tła. Każdej kontrolce przydziel własne zasoby GDI.

## Licencja

Projekt jest udostępniany na licencji MIT. Pełna treść licencji znajduje się w pliku [LICENSE](LICENSE). Licencja pozwala używać, modyfikować i rozpowszechniać kod, również komercyjnie, pod warunkiem zachowania informacji o prawach autorskich i treści licencji.

Licencje zewnętrznych bibliotek i narzędzi używanych przez projekt pozostają niezależne od licencji tego repozytorium.
