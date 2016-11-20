# SO-P2

##Temat: Prosty program współbieżny zarządzający dostępem do dysku (temat nr 4)

  Planista przyjmuje żądania wielu wątków i przydziela wątkom dostęp do dysku. Żądania wątków są kolejkowane. Kolejka planisty ma ograniczoną długość (max_disc_queue): gdy kolejka jest pełna, wątek musi zaczekać.
  
  Na początku programu tworzona jest pewna ilość wątków żądających dostępu do dysku oraz jeden wątek obsługujący żądania.  Każdy wątek żądający wydaje serię żądań do ścieżek dysku (numery ścieżek są wczytywane z kolejnych linii pliku wejściowego). Każdy wątek żądający musi poczekać z nowym żądaniem dopóki jego poprzednie żądanie nie zostanie obsłużone. Wątek żądający kończy się, gdy wszystkie żądania z danego pliku zostaną obsłużone.
  
    Żądania z kolejki są obsługiwane w porządku  SSTF (shortest seek time first) - następne obsługiwane żądanie jest żądaniem dostępu do ścieżki najbliższej aktualnemu położeniu głowicy. Ścieżka początkowa to 0.
    
    
    Dane wejściowe: maksymalna liczba żądań w kolejce,
                    lista plików wejściowych (zawierających w kolejnych liniach numery ścieżek dla danego wątku)
    Wyjście: Każdy wątek wypisuje na wyjście odpowiedni komunikat, gdy konkretne żądanie jest gotowe do obsługi.
             Po obsłudze danego żądania, wątek obsługujący wypisuje na wyjście odpowiedni komunikat.
