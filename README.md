# lab_sieci_projekt

Projekt realizowany przez:
- Krzysztof Noska-Figiel, 160054
- Rishi Dixit, 

# O projekcie

Projekt bedzie gra wieloosobowa czasu rzeczywistego, ktorej celem jest zakolorowanie 50% powierzchni mapy.

1. Rozgrywka i Mechanika Kolorowania.
    - Start: Każdy gracz rozpoczyna grę z małym, natychmiastowo zakolorowanym fragmentem terenu (bazą). 
    - Kolorowanie:
        - Wyjście z Bazy: Gracz, który opuszcza swoje zakolorowane terytorium, wchodzi w stan "kolorowania" (tworzenia linii) i staje się wrażliwy na ataki. 
        - Zakończenie: Kolorowanie kończy się, gdy gracz powróci do swojego zakolorowanego terytorium. Obszar zamknięty w pętli stworzonej przez linię staje się automatycznie zakolorowany. 
        - Możliwe Cele: Gracz może kolorować zarówno teren niezakolorowany jak i teren zakolorowany przez przeciwników. Uwaga, gracz nie moze wejsc w trym kolorowania na swoim wlasnym terenie.


2. Zasady Eliminacji i Ruchu.
    - Warunki Eliminacji (Śmierć Gracza):

        - Atak na Linię Kolorowania: Jedynym sposobem na eliminację przeciwnika jest przecięcie linii kolorowania (wejście na teren kolorowany przez niego) zanim powróci on do swojej bazy. W momencie przecięcia, atakowany gracz natychmiast przegrywa i jego całe zakolorowane terytorium znika z mapy.

        - Kolizja Graczy: W przypadku fizycznej kolizji (zderzenia) dwóch graczy, obaj gracze zostają wyeliminowani.

     - Zasady Ruchu:

        - Ciągły Ruch: Gracz nigdy nie może się zatrzymać.
        
        - Zmiana Kierunku: Jedyną możliwą akcją gracza jest ciągła zmiana kierunku ruchu.

3. Aspekty sieciowe.

    - Serwer:
        - Serwer bedzie hostowal wiele gier. Każda gra będzie miała ograniczenie do czterech graczy.  
        - Jeśli nowy klienet bedzie chcial sie dolaczyc, a kazda gra bedzie miala pelna liczbe graczy to wtedy, serwer automatycznie stworzy nową, w której gracz będzie czekał na minimalną liczbę graczy (2). 
        - Gra się rozpocznie automatycznie po osiągnięciu minimalnej liczby graczy. 
        - Gra na serwerze zakonczy sie gdy jeden z graczy zakolorowuje 50% powierzchni, lub gdy wszyscy gracze opuszcza rozgrywke. Gdy gra sie zakonczy gracze beda mogli zaglosowac za rozpoczeciem nowej rozgrywki. 
    - Klient:
        - Gracz bedzie mogl sie polaczyc do serwera i wtedy automatycznie zostanie przydzielony do pierwszej rozgrywki z miejscem.
        - Klient moze sie odlaczyc z serwerem w dowolnym momencie.

4. Dodatkowe aspekty. 

    - Gracze po wygranej lub przegranej grze będ widzieli informacje o wyniku gry oraz stosunek zajętej powierzchni do całkowitego pola mapy.
    