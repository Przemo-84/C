# linux_http_server

Podstawowy serwer HTTP dla systemu Linux, napisany w C++. Projekt był testowany na Ubuntu uruchomionym na VPS oraz w VMware i posłużył jako baza do budowy większej platformy.

## Funkcje

- serwer HTTP działający na gniazdach TCP;
- parsowanie żądań HTTP i budowanie odpowiedzi;
- routing żądań do funkcji obsługi;
- własny moduł parsowania i generowania JSON;
- przykładowy moduł SQL korzystający z ODBC i własnej puli połączeń.

W przesłanej wersji moduł SQL nie jest podłączony do głównego programu. Opis tego stanu i przykład konfiguracji SQL znajdują się w sekcji [Obsługa bazy danych](#obsługa-bazy-danych).

## Struktura projektu

| Katalog lub plik | Zawartość |
|---|---|
| `mainApp.cpp` | Punkt wejścia programu, obsługa argumentów, inicjalizacja serwera i zamykanie aplikacji. |
| `headers/mainApp.h` | Deklaracje funkcji głównego modułu. |
| `httpServer/` | Serwer TCP/HTTP, parsowanie żądań i odpowiedzi oraz routing. |
| `jsonConvert/` | Funkcje parsowania i serializacji JSON. |
| `initDataEnv/` | Struktura `dataConnections` i zapis/odczyt pliku konfiguracyjnego. |
| `sql_module/` | Moduł ODBC z pulą połączeń i przykładową funkcją korzystającą z puli. |
| `linux_http_server.sln` | Rozwiązanie Visual Studio. |
| `linux_http_server_.vcxproj` | Projekt C++ dla zdalnego systemu Linux. |
| `bin/x64/Debug/mainApp.out` | Plik wykonywalny z dołączonej konfiguracji Debug x64; po przebudowaniu użyj bieżącego pliku wynikowego. |

## Wymagania

- Visual Studio z zainstalowanymi narzędziami do rozwoju C++ dla systemu Linux;
- maszyna Linux dostępna przez SSH, np. Ubuntu na VPS albo Ubuntu w VMware;
- kompilator C++, narzędzia budowania i biblioteki wymagane przez konfigurację projektu;
- dla modułu SQL: unixODBC oraz sterownik PostgreSQL ODBC.

Konfiguracja `Debug|x64` w projekcie używa standardu C++20 i odwołuje się do biblioteki `odbc`.

## Połączenie Visual Studio z maszyną Linux

Najpierw upewnij się, że na Ubuntu działa serwer SSH. W razie potrzeby zainstaluj i uruchom go:

```bash
sudo apt update
sudo apt install openssh-server
sudo systemctl enable --now ssh
```

Ścieżka do menedżera połączeń zależy od wersji i języka interfejsu:

- Visual Studio 2022: **Tools > Options > Cross Platform > Connection Manager**.
- Visual Studio 2026 w polskiej wersji językowej: **Narzędzia > Menedżer połączeń**.

W menedżerze wybierz **Add/Dodaj** i podaj adres hosta, port SSH (zwykle `22`), nazwę użytkownika Linux oraz sposób uwierzytelniania. Następnie wybierz tę maszynę w konfiguracji projektu.

W projekcie otwórz **Właściwości projektu > Ogólne** i ustaw **Zdalny katalog główny** na katalog nadrzędny, np.:

```text
/home/uzytkownik/projekty
```

Nie dopisuj tam nazwy projektu. Visual Studio tworzy podkatalog zgodny z nazwą projektu, w tym przypadku `linux_http_server`.

W przesłanej konfiguracji Visual Studio Debug x64 używa argumentów:

```text
--dataEnv .dataEnv
```

Można je zmienić w **Właściwości projektu > Debugowanie > Argumenty programu**. Do kompilacji wybierz konfigurację `Debug|x64` i użyj polecenia **Build Solution/Kompiluj rozwiązanie**.

## Uruchomienie bez bazy danych

Program ma dwa sposoby uruchamiania. Najpierw utwórz plik konfiguracyjny, a potem uruchom serwer, wskazując ten plik.

### 1. Utworzenie pliku konfiguracji

Na maszynie Linux przejdź do katalogu z plikiem wykonywalnym:

```bash
cd /sciezka/do/linux_http_server/bin/x64/Debug
./mainApp.out --initDataEnv .dataEnv 56580
```

Argumenty oznaczają:

1. `--initDataEnv` — utworzenie pliku konfiguracji;
2. `.dataEnv` — nazwa pliku, którą później podasz przy uruchomieniu. Możesz wybrać ją samodzielnie, ale musi być podana po `--dataEnv`.
3. `56580` — port nasłuchiwania. Wybierz wolny port; `56580` jest przykładową wartością.

Program zapisuje plik binarny i kończy działanie. W tej wersji struktura zapisywana do pliku zawiera wyłącznie numer portu. Nie jest to tekstowy plik `.env` i nie zapisuje on danych logowania do bazy.

### 2. Uruchomienie serwera

Uruchom serwer z tego samego katalogu, aby względna ścieżka `.dataEnv` wskazywała utworzony plik:

```bash
./mainApp.out --dataEnv .dataEnv
```

W Visual Studio ustaw argumenty programu na:

```text
--dataEnv .dataEnv
```

Aby utworzyć lub odtworzyć plik z poziomu Visual Studio, tymczasowo ustaw:

```text
--initDataEnv .dataEnv 56580
```

Uruchom program raz, sprawdź komunikat o zapisaniu danych, a potem przywróć argument `--dataEnv .dataEnv`.

### Testowe endpointy

W aktualnym kodzie zdefiniowano dwa przykładowe endpointy:

- `GET /` — zwraca prostą stronę HTML ze statusem serwera;
- `GET /getJson` — zwraca przykładowy obiekt JSON.

Aktualny kod wiąże gniazdo z adresem `127.0.0.1`, więc połączenia przychodzące z innych maszyn nie dotrą bezpośrednio do serwera. Przy wdrożeniu za reverse proxy może to być zamierzone; przy bezpośrednim dostępie z sieci trzeba odpowiednio zmienić adres wiązania.

### Przykładowa konfiguracja Nginx

Serwer w tym projekcie nasłuchuje na `127.0.0.1`. Możesz pozostawić go dostępnym tylko lokalnie, a ruch z sieci przekazywać przez Nginx działający na tej samej maszynie. Wtedy z zewnątrz udostępniasz port Nginx (`56570` w przykładzie), a port aplikacji (`56580`) pozostaje lokalny.

Przykładowa konfiguracja dla maszyny wirtualnej:

```nginx
server {
    listen 56570;
    server_name 192.168.221.128;

    location / {
        proxy_pass http://127.0.0.1:56580;
        proxy_set_header Host $host;
        proxy_set_header X-Original-URI $request_uri;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

W tym przykładzie `server_name` zawiera adres IP maszyny wirtualnej, na której działa Nginx. Na VPS możesz podać nazwę domenową, np. `api.serwer.pl`. Upewnij się, że port Nginx jest dostępny w regułach zapory.

W konsoli programu dostępna jest komenda `close`, która zatrzymuje serwer.

## Obsługa bazy danych

### Instalacja ODBC na Ubuntu

Na maszynie Linux zainstaluj unixODBC, pliki deweloperskie oraz sterownik PostgreSQL ODBC:

```bash
sudo apt update
sudo apt install unixodbc unixodbc-dev odbc-postgresql
```

Sprawdź, czy sterownik został zarejestrowany:

```bash
odbcinst -q -d
```

Kod modułu SQL używa nazwy sterownika `PostgreSQL UNICODE`. Jeśli baza znajduje się na innym serwerze, upewnij się również, że host, port i reguły zapory pozwalają na połączenie z maszyny, na której działa aplikacja.

### Dodanie biblioteki ODBC w Visual Studio

Właściwości projektu otwórz w konfiguracji, której używasz do budowania (w załączonym projekcie: `Debug|x64`). Przejdź do **Konsolidator > Dane wejściowe > Zależności bibliotek** i dodaj ODBC na początku listy:

```text
odbc;%(LibraryDependencies)
```

W angielskim interfejsie ta ścieżka to **Linker > Input > Library Dependencies**. Ustawienie jest potrzebne, aby konsolidator dołączył bibliotekę unixODBC do programu.

### Zalecany sposób przekazania danych do `sql_module`

Sposób przechowywania konfiguracji zależy od programisty. Dla tego projektu zalecanym wariantem jest rozszerzenie istniejącego pliku konfiguracyjnego tak, aby `dataConnections` zawierało zarówno port HTTP, jak i dane połączenia z bazą. Wtedy `mainApp.cpp` może odczytać jedną strukturę i przekazać jej pola bezpośrednio do konstruktora `sql_module`.

Aby to wdrożyć:

1. Dodaj do `dataConnections` w `initDataEnv.h` pola na host bazy, nazwę bazy, użytkownika, hasło i port bazy.
2. Rozszerz deklarację i definicję `initDataEnv(...)`, aby przyjmowała i zapisywała te wartości razem z portem HTTP.
3. W `mainApp.cpp` zaktualizuj gałąź obsługującą `--initDataEnv`: tekst pomocy, sprawdzanie `argc`, indeksy argumentów, wywołanie `initDataEnv(...)` oraz `init_args_count` tak, aby uwzględniały wszystkie nowe wartości. Zwykłe uruchomienie `--dataEnv [plik]` może pozostać bez zmian.
4. Zaktualizuj odczyt pliku w trybie `--dataEnv`, a po zmianie układu binarnej struktury utwórz plik konfiguracyjny ponownie.
5. Odkomentuj w `mainApp.cpp` utworzenie `sql_module`, wywołanie `initConnection()` i obsługę kodu zwrotnego.

Konstruktor przyjmuje kolejno host, nazwę bazy, użytkownika, hasło i port bazy. Przykładowe wywołanie; nazwy pól muszą odpowiadać tym, które dodasz do struktury `dataConnections`:

```cpp
sqlModule = new sql_module(
    dataConnect.dataBase_host,
    dataConnect.dataBase_name,
    dataConnect.dataBase_user,
    dataConnect.dataBase_password,
    dataConnect.dataBase_port
);
retCode = sqlModule->initConnection();
printRetCodeMessage(retCode);
```

W obecnej paczce te pola nie są jeszcze zadeklarowane w `dataConnections`, więc sam blok inicjalizacji nie skompiluje się po odkomentowaniu bez opisanych zmian. Nie umieszczaj prawdziwego hasła w repozytorium. Jeśli nowe wartości będą podawane jako argumenty programu, hasło może być widoczne w historii poleceń lub na liście procesów; bezpieczniej pobrać je w trybie inicjalizacji z chronionego źródła niż przekazywać jawnie przez `argv`.

### Stan modułu w przesłanej wersji

Moduł `sql_module` używa ODBC i sterownika PostgreSQL o nazwie `PostgreSQL UNICODE`. Konstruktor przyjmuje host, nazwę bazy, użytkownika, hasło oraz port. Funkcja `initConnection()` próbuje utworzyć pulę ośmiu połączeń.

W `sql_module::exampleFunction()` widać pobranie połączenia z kolejki i zwrócenie go do puli. W miejscu wykonania zapytania znajduje się komentarz zastępczy — w przesłanym pliku nie ma jeszcze konkretnego zapytania SQL. Funkcja jest szkieletem, który programista może wykorzystać jako wzorzec przy implementacji własnych funkcji obsługujących zapytania.

W `mainApp.cpp` utworzenie obiektu `sql_module`, wywołanie `initConnection()` i jego zamknięcie są zakomentowane. W tej paczce serwer uruchamia się więc bez aktywnego połączenia z bazą.

Dodatkowo obecna struktura `dataConnections` przechowuje tylko port. Funkcja `--initDataEnv` nie zapisuje hosta bazy ani danych logowania. Dlatego opisany wyżej plik `.dataEnv` nie konfiguruje jeszcze SQL w tej wersji projektu.

## Zatrzymanie

W konsoli programu wpisz:

```text
close
```

## Dokumentacja Visual Studio

- [Łączenie Visual Studio z docelowym systemem Linux](https://learn.microsoft.com/en-us/cpp/linux/connect-to-your-remote-linux-computer?view=msvc-170)
- [Tworzenie projektów C++ dla Linuksa w Visual Studio](https://learn.microsoft.com/en-us/cpp/build/get-started-linux-cmake?view=msvc-170)

## Licencja

Projekt jest udostępniany na licencji MIT. Pełna treść licencji znajduje się w pliku [LICENSE](LICENSE). Licencja pozwala używać, modyfikować i rozpowszechniać kod, również komercyjnie, pod warunkiem zachowania informacji o prawach autorskich i treści licencji.

Licencje zewnętrznych bibliotek i narzędzi używanych przez projekt pozostają niezależne od licencji tego repozytorium.