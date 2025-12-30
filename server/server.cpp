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

#include <sys/epoll.h>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>

#include <iostream> // Dla std::cout
#include <limits>   // Dla std::numeric_limits
#include <cstring>
#include <errno.h>
#include <string>
#include <random>
#include <chrono>
#include <sstream>

// Nowe include'y
#include "helpers.h" // Zawiera Client, WorkerThread, GAME_GRID_SIZE, MAX_CLIENTS_PER_THREAD, etc.
#include "game_logic.h"  // Deklaracja funkcji game_logic
#include <fcntl.h>       // Dla fcntl

#define MAX_INT std::numeric_limits<int>::max()


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
        sockaddr_storage clientAddr{};
        socklen_t clientAddrSize = sizeof(clientAddr);

        // akceptujemy nowego klienta 

        // Zmienna przechowujaca pojedynczy file descryptor clienta - moze sie blokowac
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
            
            // Uruchamiamy watek game_logic
            new_worker->game_thread = std::thread(game_logic, new_worker);
            
            // Wysylamy ClientFD do nowego watku przez pipe
            if (write(new_worker->pipe_fd[1], &ClientFD, sizeof(ClientFD)) == -1) {
                perror("Blad wysylania klienta do nowego watku przez pipe");
                close(ClientFD);
                new_worker->is_running = false;
                if (new_worker->thread.joinable()) {
                    new_worker->thread.join();
                }
                if (new_worker->game_thread.joinable()) {
                    new_worker->game_thread.join();
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

    // tworzymy nowy pipe do komunikacji worker - gamelogic i dodajemy go do epolla
    epoll_event game_ev;
    game_ev.events = EPOLLIN;
    game_ev.data.fd = worker->game_pipe_fd[0];
    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, worker->game_pipe_fd[0], &game_ev) == -1) {
        perror("Blad dodawania game_pipe do epoll");
        return;
    }

    std::vector<epoll_event> events(MAX_EVENTS);

    //Petla naszego kochanego watku
    while (worker->is_running) {
        // czekamy na zdarzenia - mamy timeout 1000 zeby sprawdzac czy nasz watek jeszcze ma dzialac - przejscie przez cala petle
        int nfds = epoll_wait(worker->epoll_fd, events.data(), MAX_EVENTS, 0);
        
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

                    // ustawiamy w tryb nie blokujacy
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                    // Dodanie klienta do mapy
                    worker->clients.emplace(client_fd, Client(client_fd));

                    // Tworzenie epolli
                    epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    client_ev.data.fd = client_fd;
                    
                    if (epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
                        perror("Blad dodawania klienta do epoll");
                        close(client_fd);
                        worker->clients.erase(client_fd);
                        std::lock_guard<std::mutex> lock(worker->mtx);
                        worker->client_count--;
                    } else {
                        // DODAJ TO: Wysyłamy komunikat do game_logic o nowym graczu
                        GameMessage msg;
                        msg.type = MessageType::NEW_PLAYER;
                        msg.client_fd = client_fd;
                        write(worker->control_pipe_fd[1], &msg, sizeof(msg));
                        std::cout << "Wyslano komunikat NEW_PLAYER do game_logic (fd=" << client_fd << ")" << std::endl;
                    }
                }
            }
            // Obsluga danych z game_logic - odbiór macierzy
            else if (events[i].data.fd == worker->game_pipe_fd[0]) {
                size_t matrix_size;
                ssize_t n = read(worker->game_pipe_fd[0], &matrix_size, sizeof(matrix_size));
                
                if (n == sizeof(matrix_size)) {
                    // Buforek na macierz
                    std::vector<char> matrix_buffer(matrix_size);
                    
                    // Czytanie macierzy
                    size_t total_read = 0;
                    while (total_read < matrix_size) {
                        ssize_t bytes_read = read(worker->game_pipe_fd[0], 
                                                matrix_buffer.data() + total_read, 
                                                matrix_size - total_read);
                        if (bytes_read <= 0) break;
                        total_read += bytes_read;
                    }
                    
                    if (total_read == matrix_size) {
                        std::string matrix(matrix_buffer.begin(), matrix_buffer.end());
                        std::cout << "Odebrano macierz z game_logic (" << matrix_size 
                                << " bajtow), rozsylam do " << worker->clients.size() 
                                << " klientow" << std::endl;
                        
                        // Wysylamy macierz do wszystkich klientow z poprawną obsługą błędów
                        for (auto& [fd, client] : worker->clients) {
                            size_t total_sent = 0;
                            const char* data = matrix.c_str();
                            
                            // Wysyłamy w pętli, obsługując częściowe wysyłki
                            while (total_sent < matrix_size) {
                                ssize_t sent = send(fd, data + total_sent, 
                                                matrix_size - total_sent, 
                                                MSG_NOSIGNAL);
                                
                                if (sent == -1) {
                                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        // Bufor pełny, czekamy chwilę
                                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                        continue;
                                    } else {
                                        // Prawdziwy błąd
                                        std::cout << "Blad wysylania do klienta " << fd 
                                                << ": " << strerror(errno) << std::endl;
                                        break;
                                    }
                                } else if (sent == 0) {
                                    std::cout << "Polaczenie zamkniete dla klienta " << fd << std::endl;
                                    break;
                                } else {
                                    total_sent += sent;
                                }
                            }
                            
                            if (total_sent == matrix_size) {
                                std::cout << "Wyslano macierz do klienta " << fd 
                                        << " (" << total_sent << " bajtow)" << std::endl;
                            } else {
                                std::cout << "Wyslano tylko " << total_sent << "/" 
                                        << matrix_size << " bajtow do klienta " << fd << std::endl;
                            }
                        }
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
                    char temp_buffer[BUFFER_SIZE]; // Tymczasowy bufor do odczytu z gniazda
                    ssize_t bytes_read = read(fd, temp_buffer, BUFFER_SIZE);

                    if (bytes_read == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            close_conn = true;
                        }
                    } else if (bytes_read == 0) {
                        close_conn = true;
                    } else {
                        // Dodaj odczytane dane do bufora klienta
                        it->second.buffer.append(temp_buffer, bytes_read);

                        // Przetwarzaj znaki z bufora klienta
                        while (!it->second.buffer.empty()) {
                            char received_char = it->second.buffer.front(); // pobieramy znaj
                            it->second.buffer.erase(0, 1); // usuwamy go od buffora

                            if (received_char == 'a' || received_char == 'd' || received_char == 'w') {
                                GameMessage msg;
                                msg.type = MessageType::PLAYER_MOVE;
                                msg.client_fd = fd;
                                msg.move_data = received_char;
                                write(worker->control_pipe_fd[1], &msg, sizeof(msg));
                                if (DEBUGGING){
                                std::cout << "Wyslano komunikat PLAYER_MOVE do game_logic (fd=" << fd << ", move=" << received_char << ")" << std::endl;
                                }
                            } else {
                                // nie potrzebujemy juz tego ale zostawiam do debuggowania
                               // std::cout << "Odebrano nieznany znak od klienta " << fd << ": " << received_char << std::endl;
                            }
                        }
                    }
                }

                // Obsluga rozlaczenia lub bledu
                if (close_conn || (events[i].events & (EPOLLERR | EPOLLHUP))) {
                    // Wysyłamy komunikat do game_logic o usunięciu gracza
                    GameMessage msg;
                    msg.type = MessageType::REMOVE_PLAYER;
                    msg.client_fd = fd;
                    write(worker->control_pipe_fd[1], &msg, sizeof(msg));
                    std::cout << "Wyslano komunikat REMOVE_PLAYER do game_logic (fd=" << fd << ")" << std::endl;
                    
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

    // konczymy watki jesli jeszcze dzialaja

    if (worker->game_thread.joinable()) {
        worker->game_thread.join();
        std::cout << "Watek game_logic zostal polaczony z watkiem roboczym." << std::endl;
    }

    // usuniecie watku z listy watkow
    {
        std::lock_guard<std::mutex> lock(workers_mutex);
        for (auto it = workers.begin(); it != workers.end(); ++it) {
            if (*it == worker) {
                if (worker->thread.joinable()) {
                    worker->thread.detach();
                }
                workers.erase(it);
                std::cout << "Usunieto watek roboczy. Liczba watkow: " << workers.size() << std::endl;
                break;
            }
        }
    }

}