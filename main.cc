#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <vector>

using namespace std;

sem_t print;	//semafor binarny synchronizujący wypisywanie na wyjście
sem_t* request;	//tablica semaforów binarnych wątków żądających dostępu do dysku (tylko 1 żądanie danego wątku może się znajdować w kolejce w danej chwili)
int max_disk_queue;	//maksymalna liczba żądań w "kolejce" (wektorze) podana jako argument programu
int num_of_living_rth;	//liczba aktualnie żywych wątków żądających
sem_t living_sec;	//semafor binarny synchronizujący zmienianie i odczytywanie wartości zmiennej globalnej num_of_living_rth
sem_t max_disk_queue_sec;	//semafor "wpuszczający" do "kolejki" maksymalnie liczbę max_disk_queue wątków podaną jako argument programu
sem_t queue_sec;	//semafor binarny synchronizujący dodawanie żądań do "kolejki" (wektora) i obsługiwanie żądań z "kolejki" (w danej chwili tylko jedno żądanie może być dodawane lub obsługiwane)
sem_t full;	//semafor ustawiany na 1, gdy "kolejka" jest pełna (planista czeka aż "kolejka" będzie pełna, żeby zminimalizować średni seek time)

//struktura przekazywana jako argument do funkcji wątku żądającego
struct Argument{
	int i;	//numer wątku
	string filename;	//nazwa pliku wejściowego z numerami ścieżek dla danego wątku
	Argument(int j, string f)
	{
		i = j;
		filename = f;
	}
};

//w "kolejce"(wektorze) przechowujemy numer wątku żądającego dostępu do dysku wraz z numerem ścieżki (do której ten wątek chce uzyskać dostęp)
struct data{
	int threadnum;
	int tracknum;
};

vector<data> queue;	//"kolejka" przechowująca żądania (numery wątków żądających wraz z numerami ścieżek)

//wątek żądający
void* requester(void *arg)
{
	Argument* a = (Argument*)arg;
	data d;
	d.threadnum = a->i;	
	string f = a->filename;
	ifstream file;
	file.open(f.c_str());	//otwarcie pliku, którego nazwa była przekazana przez argument
	if(file.fail()){
		cerr<<"Error opening file"<<endl;
		exit(-1);
	}
	int x;
	file>>x;	//wczytanie pierwszego numeru ścieżki z pliku
	while (!file.eof()) {
		d.tracknum = x;
		sem_wait(&(request[d.threadnum]));	//wątek czeka aż poprzednie jego żądanie zostanie obsłużone
		sem_wait(&max_disk_queue_sec);	//wątek czeka aż "kolejka" nie będzie pełna
		sem_wait(&queue_sec);	//sekcja krytyczna operacji na kolejce
		queue.push_back(d);	//dodanie żądania do kolejki
		sem_wait(&print);
		cout<<"Żądanie wątku nr "<<d.threadnum<<" dostępu do ścieżki "<<x<<endl;	//wypisanie odpowiedniego komunikatu
		sem_post(&print);
		sem_wait(&living_sec);
		if(queue.size() == fabs(min(max_disk_queue, num_of_living_rth)))	//jeśli kolejka jest pełna, planista jest budzony
			sem_post(&full);
		sem_post(&living_sec);
		sem_post(&queue_sec);
		file>>x;	//wczytanie kolejnego numeru ścieżki z pliku
	}
	file.close();
	sem_wait(&(request[d.threadnum]));	//wątek czeka aż ostatnie jego żądanie zostanie obsłużone
	sem_wait(&living_sec);
	num_of_living_rth--;	//zmniejszenie liczby żywych wątków
	if(queue.size() == fabs(min(max_disk_queue, num_of_living_rth)))	//jeśli kolejka jest teraz pełna, planista jest budzony
			sem_post(&full);
	sem_post(&living_sec);
	return NULL;
}

//wątek planisty
void* service(void *arg)
{
	long head_pos = (long)arg;
	data d;
	sem_wait(&print);	
	cout<<"Pozycja głowicy: "<<head_pos<<endl;	//wyświetlenie początkowej pozycji głowicy
	sem_post(&print);
	unsigned int temp;	//zmienna pomocnicza
	while(num_of_living_rth != 0)
	{
		sem_wait(&full);	//planista czeka aż "kolejka" będzie pełna
		sem_wait(&living_sec);
		if(num_of_living_rth == 0)	//jeżeli nie ma już żywych wątków żądających, to znaczy, że planista nie ma już żądań do obsłużenia
			break;
		sem_post(&living_sec);
		sem_wait(&queue_sec);
		temp = 0;
		for(unsigned int k=1; k<queue.size(); k++)	//wyszukiwanie w kolejce żądania o numerze ścieżki najbliższym aktualnej pozycji głowicy
		{
			if(fabs(head_pos - queue[temp].tracknum) > fabs(head_pos - queue[k].tracknum))
				temp = k;
		}
		head_pos = queue[temp].tracknum;	//zmiana pozycji głowicy
		d = queue[temp];
		sem_wait(&print);
		cout<<"Obsłużenie żądania wątku nr "<<d.threadnum<<" dostępu do ścieżki nr "<< d.tracknum<<endl;
		cout<<"Pozycja głowicy: "<<head_pos<<endl;
		sem_post(&print);
		queue[temp] = queue.back();	//przesunięcie ostatniego elementu z "kolejki" na pozycję żądania, które właśnie zostało obsłużone
		queue.pop_back();
		sem_post(&queue_sec);
		sem_post(&max_disk_queue_sec);
		sem_post(&(request[d.threadnum]));	//obudzenie odpowiedniego wątku
	}
	return NULL;	
}

int main(int argc, char* argv[])
{
	long head_position = 0;	//pozycja głowicy
	int num_of_rthreads = argc-2;	//liczba wątków żądających jest równa liczbie plików podanych jako argumenty programu
	max_disk_queue = atoi(argv[1]);	//maksymalna długość "kolejki"
	int error = 0;
	sem_init(&queue_sec, 0, 1);	//semafor binarny synchronizujący dodawanie żądań do "kolejki" (wektora) i obsługiwanie żądań z "kolejki" (w danej chwili tylko jedno żądanie może być dodawane lub obsługiwane)
	sem_init(&print, 0, 1);	//semafor binarny synchronizujący wypisywanie na wyjście
	sem_init(&living_sec, 0, 1);	//semafor binarny synchronizujący zmienianie i odczytywanie wartości zmiennej globalnej num_of_living_rth
	sem_init(&max_disk_queue_sec, 0, max_disk_queue);	//semafor "wpuszczający" do "kolejki" maksymalnie liczbę max_disk_queue wątków podaną jako argument programu
	sem_init(&full, 0, 0);	//semafor ustawiany na 1, gdy "kolejka" jest pełna (planista czeka aż "kolejka" będzie pełna, żeby zminimalizować średni seek time)
	pthread_t s;	//wątek planisty
	pthread_t requester_threads[num_of_rthreads];	//tablica wątków żądających
	string files[num_of_rthreads];	//tablica nazw plików podanych na wejściu
	for(int k=0; k<num_of_rthreads; k++)
		files[k] = argv[k+2];
	request = (sem_t*)malloc(num_of_rthreads*sizeof(sem_t));	//alokacja pamięci na tablicę semaforów dla wątków żądających
	
	for(int i=0; i<num_of_rthreads; i++)	
	{
		sem_init(&(request[i]), 0, 1);	//semafor binarny i-tego wątku żądającego (tylko 1 żądanie danego wątku może się znajdować w kolejce w danej chwili)
		error = pthread_create(&requester_threads[i], NULL, requester, (void *)new Argument(i, files[i]));	//tworzenie i-tego wątku żądającego dostępu do dysku 
		//(przez argument przekazujemy numer wątku i nazwę pliku z numerami ścieżek dla danego wątku)
		if (error)
		{
			sem_wait(&print);
			printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
	}
	
	sem_wait(&living_sec);
	num_of_living_rth = num_of_rthreads;	//ustawienie początkowej wartości liczby żywych wątków żądających
	sem_post(&living_sec);

	error = pthread_create(&s, NULL, service, (void*)head_position);	//tworzenie wątku planisty (pozycja głowicy jest przekazywana przez argument)
	if (error)
		{
			sem_wait(&print);
			printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
		
	for(int j=0; j<num_of_rthreads; j++)
	{
		error = pthread_join(requester_threads[j], NULL);
		if(error)
		{
			printf("BŁĄD (przy oczekiwaniu na zakończenie wątków): %s\n", strerror(error));
			exit(-1);
		}
	}
	error = pthread_join(s, NULL);
	if(error)
	{
		printf("BŁĄD (przy oczekiwaniu na zakończenie wątków): %s\n", strerror(error));
		exit(-1);
	}
	return 0;
}
