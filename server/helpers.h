#ifndef HELPERS_H
#define HELPERS_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include "player.h"  // DODAJ TO - teraz możemy użyć Player

// Stałe
const int GAME_GRID_SIZE = 20;
const int MAX_CLIENTS_PER_THREAD = 4;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;
const int GAME_REFRESH_INTERVAL = 2000;
const bool DEBUGGING = true;

// Typy komunikatów między serwerem a game_logic
enum class MessageType : char {
    NEW_PLAYER = 'N',
    REMOVE_PLAYER = 'R',
    MATRIX_UPDATE = 'M',
    PLAYER_MOVE = 'P'
};

// Struktura komunikatu
struct GameMessage {
    MessageType type;
    int client_fd;
    char move_data;
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
    int pipe_fd[2];
    int game_pipe_fd[2];
    int control_pipe_fd[2];
    std::thread thread;
    std::thread game_thread;
    std::mutex mtx;
    std::unordered_map<int, Client> clients;
    
    // Stan gry dla tego wątku:
    std::vector<std::vector<char>> matrix_grid;
    std::vector<std::vector<char>> matrix_before_coloring; // macierz ale bez zadnych trybow kolorowania
    std::vector<Player> players;  // Teraz to zadziała!
    std::vector<bool> color_used;
    
    WorkerThread();
    ~WorkerThread();
};

#endif // HELPERS_H