Jesteś senior software architectem i bardzo doświadczonym inżynierem C/C++ oraz systemów czasu rzeczywistego. Twoim zadaniem jest przekształcenie istniejącego projektu APRS Navigator w architekturę opartą o niezależny engine bez GUI i bez klawiatury, zgodnie z poniższą specyfikacją.

DOSTARCZONE MATERIAŁY:
1. Instrukcja obsługi APRS Navigator, opisująca wszystkie funkcje urządzenia.
2. Specyfikacja architektury `aprs_engine_architecture.md`.
3. Nagłówek API `aprs_engine_v2.h`.
4. Specyfikacja protokołu WebSocket/JSON `aprs_engine_ws_protocol.md`.

CEL:
- Wyodrębnij cały rdzeń APRS Navigator do osobnego engine.
- Usuń zależność od GUI i klawiatury z logiki domenowej.
- Zachowaj pełną funkcjonalność urządzenia opisaną w instrukcji.
- Udostępnij silnik przez publiczne API w C.
- Zapewnij obsługę wielu klientów jednocześnie.
- Zapewnij push zdarzeń do klientów przez event bus oraz WebSocket.
- Utrzymaj możliwość działania jako biblioteka i jako proces serwera.

WYMAGANIA FUNKCJONALNE:
Silnik musi obsługiwać co najmniej:
- listę stacji z aktualizacją, sortowaniem, markowaniem i szczegółami,
- radar jako model danych, nie UI,
- odbiór, wysyłanie, przechowywanie i retry wiadomości,
- GPS: GPGGA, GPRMC, tryby OFF/ON/FIX,
- pozycję użytkownika i pozycję raportowaną,
- tryby AUTO / STATIC / GPS dla pozycji,
- 3 zapisane lokalizacje,
- ręczne wpisanie pozycji,
- capture pozycji stacji do własnej pozycji,
- beaconing z 3 statusami i 3 ścieżkami oraz rotacją,
- manual send beacon,
- digipeater z 4 aliasami, typami simple/flood/trace,
- timing digi: dupetime, dupedelay,
- listę ostatnich powtórzeń digi,
- TX lock dla beacon/digi/query,
- filtry ramek: unique only, tcpip, invalid,
- station announce,
- raw frame monitor,
- kopiowanie, edycję i wysyłanie jednej ramki,
- zdalne sterowanie jednorazowymi kluczami,
- statystyki godzinne i 20-godzinne,
- konfigurację TRX: TXDelay, TXHeader, bitdelay, Rxtune, squelch,
- konfigurację dźwięków jako abstrakcyjnych eventów.

ARCHITEKTURA:
- Engine ma być jedynym właścicielem stanu domenowego.
- GUI, CLI i Web UI mają być osobnymi klientami.
- Wewnętrzna komunikacja ma używać serialized command queue.
- Event bus ma rozsyłać zdarzenia do wielu subskrybentów.
- Transporty HTTP i WebSocket mają być tylko adapterami.
- Kod rdzenia nie może mieć zależności od UI ani od konkretnego transportu.

ZASADY IMPLEMENTACJI:
1. Najpierw wydziel model danych i logikę z istniejącego kodu.
2. Usuń wywołania renderowania, wejścia z klawiatury i ekranów z core.
3. Zastąp je eventami i komendami.
4. Zachowaj wszystkie zachowania opisane w instrukcji.
5. Zadbaj o ABI-stabilność publicznego C API.
6. Używaj struktury wersjonowanej i fixed-size buffers tam, gdzie to potrzebne.
7. Nie twórz logiki „na skróty”; jeśli manual opisuje funkcję, ma ona istnieć w engine.
8. Zaimplementuj testy jednostkowe dla każdego modułu.

PODZIAŁ NA MODUŁY:
- frame_ingest
- station_manager
- position_manager
- beacon_manager
- message_manager
- digi_engine
- gps_manager
- stats_engine
- raw_frame_editor
- remote_control
- trx_config
- notification_manager
- event_bus
- http_adapter
- websocket_adapter
- ui_adapter (tylko klient, nie core)

PUBLICZNE API:
Korzystaj z nagłówka `aprs_engine_v2.h` jako kontraktu. Nie zmieniaj nazw funkcji bez silnego powodu. Jeśli trzeba rozszerzyć API, rób to kompatybilnie wstecz.

PROTOKÓŁ SIECIOWY:
- Użyj protokołu JSON z `aprs_engine_ws_protocol.md`.
- Wspieraj hello, subscribe, unsubscribe, snapshot, command, ack, error, event.
- Wspieraj filtrowanie topiców.
- Wspieraj wielu klientów naraz.

WYMAGANIA DLA KODU:
- Kod ma być czysty, modularny i testowalny.
- Unikaj globali.
- Unikaj hardcode’owania danych UI.
- Wszystkie funkcje sterujące mają zwracać status.
- Dodaj logowanie i obsługę błędów.
- Dodaj miejsce na persystencję konfiguracji.
- Dodaj stuby dla niezaimplementowanych jeszcze transportów, ale nie pomijaj core.

KOLEJNOŚĆ PRACY:
1. Zaproponuj docelową strukturę katalogów.
2. Zidentyfikuj pliki, które trzeba rozdzielić.
3. Zaprojektuj modele danych.
4. Zaimplementuj core.
5. Zaimplementuj event bus.
6. Zaimplementuj publiczne API C.
7. Zaimplementuj transport HTTP.
8. Zaimplementuj transport WebSocket.
9. Na końcu zrób adapter GUI jako osobny klient.
10. Dodaj testy i przykładowe konfiguracje.

OCZEKIWANY FORMAT ODPOWIEDZI:
Najpierw przygotuj plan refaktoru i strukturę katalogów. Następnie wygeneruj kod krok po kroku. Jeśli coś jest niejednoznaczne, wybierz rozwiązanie najbliższe instrukcji obsługi i opisz założenia.