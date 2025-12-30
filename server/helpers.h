#ifndef HELPERS_H
#define HELPERS_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>

// Stałe
const int GAME_GRID_SIZE = 20;
const int MAX_CLIENTS_PER_THREAD = 4;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;
const int GAME_REFRESH_INTERVAL = 2000; // co ile ms ma gra sie odswiezac
const bool DEBUGGING = true;

// Typy komunikatów między serwerem a game_logic
enum class MessageType : char {
    NEW_PLAYER = 'N',      // Nowy gracz
    REMOVE_PLAYER = 'R',   // Usuń gracza
    MATRIX_UPDATE = 'M',   // Aktualizacja macierzy
    PLAYER_MOVE = 'P'      // Ruch gracza (a lub d)
};

// Struktura komunikatu
struct GameMessage {
    MessageType type;
    int client_fd;
    char move_data; // Dodane pole do przechowywania ruchu gracza
};

// Struktura Client
struct Client {
    int fd;
    std::string buffer;
    
    Client(int client_fd) : fd(client_fd), buffer("") {}
};

// Struktura WorkerThread
struct WorkerThread {
    int client_count;
    bool is_running;
    int epoll_fd;
    int pipe_fd[2];         // Komunikacja main -> worker (nowi klienci)
    int game_pipe_fd[2];    // Komunikacja game_logic -> worker (macierz)
    int control_pipe_fd[2]; // Komunikacja worker -> game_logic (dodaj/usuń gracza)
    
    std::thread thread;
    std::thread game_thread;
    std::mutex mtx;
    
    std::unordered_map<int, Client> clients;
    
    WorkerThread();
    ~WorkerThread();
};

#endif // HELPERS_H