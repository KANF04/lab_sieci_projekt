/*
Celem tego pliku jest stworzenie serwera.
Kazdy serwer bedzie tworzyl nowe watki ktore beda zajmowaly sie komunikacja na poziomie gry.
Co oznacza ze kazdy nowy watek bedzie tworzyl minimum 2 nowe watki do komunikacji z klientem.
*/

#include <cstdio>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_set>

#include <poll.h>
#include <sys/epoll.h>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fcntl.h>

/* Dla dodania maxymalnego limitu MAX_INT*/
#include <iostream>
#include <limits>

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <string>
/* Zmienna przechowujaca maksymalnego inta*/
#define MAX_INT std::numeric_limits<int>::max()
#define MAX_CLIENTS_PER_THREAD 4
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024


// Struktura reprezentujaca pojedynczego klienta
struct Client {
    int fd;                       // Deskryptor socketa klienta
    std::string buffer;           // Bufor na dane od klienta
    
    Client(int socket_fd) : fd(socket_fd) {}
};

// Struktura reprezentujaca watek roboczy
struct WorkerThread {
    std::thread thread;           // Sam watek
    int client_count;             // Liczba klientow obslugianych przez ten watek
    bool is_running;              // Czy watek dziala
    std::mutex mtx;               // Mutex do synchronizacji dostepu do client_count
    int pipe_fd[2];               // Pipe do komunikacji: [0] - czytanie, [1] - pisanie
    int epoll_fd;                 // Deskryptor epoll dla tego watku
    std::unordered_map<int, Client> clients;  // Mapa klientow: fd -> Client
    
    WorkerThread() : client_count(0), is_running(false), epoll_fd(-1) {
        // Tworzymy pipe do komunikacji z watkiem
        if (pipe(pipe_fd) == -1) {
            perror("Blad tworzenia pipe");
        }
    }
    
    ~WorkerThread() {
        // Zamykamy pipe przy usuwaniu
        if (pipe_fd[0] != -1) close(pipe_fd[0]);
        if (pipe_fd[1] != -1) close(pipe_fd[1]);
        if (epoll_fd != -1) close(epoll_fd);
    }
};

// Funkcja ktora bedzie wykonywana przez watek roboczy
void worker_thread_function(std::shared_ptr<WorkerThread> worker);


/*Funkcja getaddrinfo_socket_bind_listen ma na celu utworzenie i skonfigurowanie gniazda serwera.

args:
    - int argc - liczba argumentów
    - const char *const *argv - tablica argumentów

return:
    - int - deskryptor gniazda
*/
int getaddrinfo_socket_bind_listen(int argc, const char *const *argv);

// handles SIGINT - wyjscie przy ctrl+c
void ctrl_c(int);


// Zmienne globalne

// Zmienna przechowujaca socket serwera -file descryptor
int servFd;
// Zmienan przechowujaca file descryptor klientow
std::unordered_set<int> clientFds;
// Lista watkow roboczych
std::vector<std::shared_ptr<WorkerThread>> workers;
// Mutex do synchronizacji dostepu do listy watkow
std::mutex workers_mutex;

/*
Funkcja main, funckja uruchamiajaca caly serwer.
Funkcja przyjmuje jako argumenty 1 port lub domyslnie daje port: "4444"

    args:
        - int argc - liczba argumentow przyjmowanych przy uruchamianiu
        - char* argv[] - tablica argumentow.
*/
int main(int agrc, char* argv[]) {
    // zmienna przechowujaca file descriptor (socket) serwera
    int sfd = getaddrinfo_socket_bind_listen(agrc, argv);
    if (sfd == -1){
        std::cout<<"Blad w probie utworzenia socketu";
        return -1;
    }

    std::cout << "Serwer nasluchuje na porcie..." << std::endl;

    // set up ctrl+c handler for a graceful exit
    signal(SIGINT, ctrl_c);
    // prevent dead sockets from throwing pipe errors on write
    signal(SIGPIPE, SIG_IGN);

    // Petla w ktorej bedzie dziala sie cala logika glownego serwera
    while (true)
    {
        // Zmienna przechowujaca informacje o kliencie
        sockaddr_storage clientAddr{0};
        socklen_t clientAddrSize = sizeof(clientAddr);

        // akceptujemy nowego klienta 

        // Zmienna przechowujaca pojedynczy file descryptor clienta
        int ClientFD = accept(servFd, (sockaddr *)&clientAddr, &clientAddrSize);
        if (ClientFD == -1) {
            perror("Blad w accept ;(");
            continue;
        }

        std::cout << "Nowe polaczenie! ClientFD: " << ClientFD << std::endl;

        // Skoro przyszedl klient musimy sprawdzic czy istnieje watek ktory moze go przyjac
        
        // Blokujemy dostep do listy watkow
        std::lock_guard<std::mutex> lock(workers_mutex);
        
        // Zmienna wskazujaca czy znalezlismy wolny watek
        bool found_free_worker = false;
        
        // Przeszukujemy liste watkow
        for (auto& worker : workers) {
            // Blokujemy dostep do licznika klientow tego watku
            std::lock_guard<std::mutex> worker_lock(worker->mtx);
            
            // Sprawdzamy czy watek ma wolne miejsce (mniej niz 4 klientow)
            if (worker->client_count < MAX_CLIENTS_PER_THREAD) {
                std::cout << "Znaleziono wolny watek! Klienci: " << worker->client_count << "/" << MAX_CLIENTS_PER_THREAD << std::endl;
                
                // Wysylamy ClientFD do watku przez pipe
                if (write(worker->pipe_fd[1], &ClientFD, sizeof(ClientFD)) == -1) {
                    perror("Blad wysylania klienta przez pipe");
                    close(ClientFD);
                } else {
                    // Zwiekszamy licznik klientow
                    worker->client_count++;
                    std::cout << "Przypisano klienta do watku. Nowa liczba klientow: " << worker->client_count << std::endl;
                }
                
                found_free_worker = true;
                break;  // Wyszukiwanie zakonczone
            }
        }
        
        // Jesli nie znalezlismy wolnego watku, tworzymy nowy
        if (!found_free_worker) {
            std::cout << "Wszystkie watki zajete! Tworze nowy watek..." << std::endl;
            
            // Tworzymy nowy obiekt WorkerThread
            auto new_worker = std::make_shared<WorkerThread>();
            new_worker->is_running = true;
            new_worker->client_count = 1;  // Bedzie obslugiwac tego klienta
            
            // Uruchamiamy watek
            new_worker->thread = std::thread(worker_thread_function, new_worker);
            
            // Wysylamy ClientFD do nowego watku przez pipe
            if (write(new_worker->pipe_fd[1], &ClientFD, sizeof(ClientFD)) == -1) {
                perror("Blad wysylania klienta do nowego watku przez pipe");
                close(ClientFD);
                new_worker->is_running = false;
                if (new_worker->thread.joinable()) {
                    new_worker->thread.join();
                }
            } else {
                // Dodajemy nowy watek do listy
                workers.push_back(new_worker);
                std::cout << "Utworzono nowy watek! Liczba watkow: " << workers.size() << std::endl;
            }
        }
        
        // TODO: 
        // Tutaj dodam logike usuwania watkow ktorych client_count == 0
        
    }
    

    return 0;
}

int getaddrinfo_socket_bind_listen(int argc, const char *const *argv) { 
    //if (argc != 2) error(1, 0, "Usage: %s <port>", argv[0]);
    // zmienna przechowujaca informacje o porcie serwera
    const char *sport;
    if (argc != 2) {
        sport = "4444";
    } else {
        sport = argv[1];
    }
    // Zmienna ktora sluzy stworzeniu zmienne res ktora poslozy do stworzenia socket'u
    addrinfo hints{};
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    
    // Zmienna sluzaca stworzeniu socketu
    addrinfo *res;
    // Zmienna sprawdzajaco sukces funkcji getaddinfo
    int rv = getaddrinfo(nullptr, sport, &hints, &res);
    if (rv) 
        //error(1, 0, "getaddrinfo: %s", gai_strerror(rv));
        return -1;
    // Tworzymy socket
    servFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // sprawdzenie bledu
    if (servFd == -1) 
        //error(1, errno, "socket failed");
        return -1;
    // ustawienie serwera aby na tym samym porcie mozna bylo odrazu dokonac binda nawet jesli port jest w stanie TIME_WAIT
    const int one = 1;
    setsockopt(servFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // dokonanie binda wraz z kontrola bledow
    if (bind(servFd, res->ai_addr, res->ai_addrlen))
        //error(1, errno, "bind failed");
        return -1;

    // wlaczenie nasluchiwania
    if (listen(servFd, MAX_INT))
        //error(1, errno, "listen failed");
        return -1;
    
    return servFd;

}

void ctrl_c(int) {
    const char quitMsg[] = "[Server shutting down. Bye!]\n";
    
    // Zamykamy wszystkich klientow
    for (int clientFd : clientFds) {
        shutdown(clientFd, SHUT_RDWR);
        close(clientFd); 
    }
    
    // Zamykamy wszystkie watki robocze
    {
        std::lock_guard<std::mutex> lock(workers_mutex);
        for (auto& worker : workers) {
            worker->is_running = false;
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
    }
    
    close(servFd);
    printf("Closing server\n");
    exit(0);
}


void worker_thread_function(std::shared_ptr<WorkerThread> worker) {
    std::cout << "Watek roboczy uruchomiony!" << std::endl;
    
    // tworzymy epoll ;)
    worker->epoll_fd = epoll_create1(0);
    if (worker->epoll_fd == -1) {
        perror("Blad tworzenia epoll");
        return;
    }

    // pipe do epoll'a tak abysmy mogli odiberac klientow 
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = worker->pipe_fd[0];
    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, worker->pipe_fd[0], &ev) == -1) {
        perror("Blad dodawania pipe do epoll");
        return;
    }

    
    std::vector<epoll_event> events(MAX_EVENTS);

    //Petla naszego kochanego watku
    while (worker->is_running) {
        // Czekamy na zdarzenia - mamy timeout 1000 zeby sprawdzac czy nasz watek jeszcze ma dzialac - przejscie przez cala petle
        int nfds = epoll_wait(worker->epoll_fd, events.data(), MAX_EVENTS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("Blad epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            // Obsluga komunikacji z watkiem glownym  -- dodanie nowego klienta
            if (events[i].data.fd == worker->pipe_fd[0]) {
                int client_fd;
                ssize_t n = read(worker->pipe_fd[0], &client_fd, sizeof(client_fd));
                if (n == sizeof(client_fd)) {
                    // Ustawienie socketu w tryb nieblokujacy (wymagane dla epoll)
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                    // Dodanie klienta do mapy -- nie wiem czy sie przyda ale to poki co tutaj zostawiam
                    worker->clients.emplace(client_fd, Client(client_fd));

                    // Rejestracja w epoll: czytanie (EPOLLIN) i pisanie (EPOLLOUT)
                    // Uzywamy EPOLLET (Edge Triggered), aby uniknac busy-loop przy EPOLLOUT
                    epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    client_ev.data.fd = client_fd;
                    
                    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
                        perror("Blad dodawania klienta do epoll");
                        close(client_fd);
                        worker->clients.erase(client_fd);
                        std::lock_guard<std::mutex> lock(worker->mtx);
                        worker->client_count--;
                    }
                }
            } 
            // Obsluga klientow
            else {
                int fd = events[i].data.fd;
                auto it = worker->clients.find(fd);
                if (it == worker->clients.end()) continue;

                bool close_conn = false;

                // Czytanie danych od klienta
                if (events[i].events & EPOLLIN) {
                    char buffer[BUFFER_SIZE];
                    // Czytamy tak dlugo az wszystko co mozliwe zostalo odczytane
                    
                    while (true) {
                        ssize_t count = read(fd, buffer, BUFFER_SIZE);
                        if (count == -1) {
                            if (errno != EAGAIN && errno != EWOULDBLOCK) close_conn = true;
                            break;
                        } else if (count == 0) {
                            close_conn = true;
                            break;
                        } else {
                            it->second.buffer.append(buffer, count);
                        }
                        // Wyswietlenie z bufora w terminalu
                        std::cout.write(buffer, count);
                    }
                }

                // Wysylanie danych - testowa wersja wysylajaca jeden znak
                if (!close_conn && (events[i].events & EPOLLOUT)) {
                    char ch[] = "X\n"; // Dowolny znak
                    ssize_t sent = send(fd, &ch, 1, MSG_DONTWAIT | MSG_NOSIGNAL);
                    if (sent == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) close_conn = true;
                    }
                }

                // Obsluga rozlaczenia lub bledu
                if (close_conn || (events[i].events & (EPOLLERR | EPOLLHUP))) {
                    epoll_ctl(worker->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    std::cout<<"\nKlient sie odlaczyl. No poprostu tragedia!\n";
                    close(fd);
                    worker->clients.erase(fd);
                    
                    std::lock_guard<std::mutex> lock(worker->mtx);
                    worker->client_count--;
                }
            }
        }
    }
    
    std::cout << "Watek roboczy zakonczony!" << std::endl;
}