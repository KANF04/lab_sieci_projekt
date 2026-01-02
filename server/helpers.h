#ifndef HELPERS_H
#define HELPERS_H

#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include "player.h"

// Stałe
const int GAME_GRID_SIZE = 20;
const int MAX_CLIENTS_PER_THREAD = 4;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;
const int GAME_REFRESH_INTERVAL = 500;
const bool DEBUGGING = true;

// Typy komunikatów między serwerem a game_logic
enum class MessageType : char {
    NEW_PLAYER = 'N',
    REMOVE_PLAYER = 'R',
    MATRIX_UPDATE = 'M',
    PLAYER_MOVE = 'P',
    RESPWAN_PLAYER = 'S',
    NEW_GAME_REQUEST = 'Y'  // NOWE: Żądanie nowej gry
};

// Typy komunikatów od game_logic do worker_thread
enum class GameLogicMessageType : char {
    PLAYER_ID_ASSIGNED = 'I',
    PLAYER_DIED = 'D',
    PLAYER_WAITING_RESPAWN = 'R',
    MATRIX_UPDATE = 'M',
    PLAYER_WON = 'W',         
    PLAYER_LOST = 'L'         
};

// Struktura komunikatu od game_logic do worker_thread
struct GameLogicToWorkerMsg {
    GameLogicMessageType type;
    int player_id;
    int client_fd;
    size_t data_length;
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
    std::vector<std::vector<char>> matrix_before_coloring;
    std::vector<Player> players;
    std::vector<bool> color_used;
    bool game_ended;           // NOWE: Czy gra się zakończyła
    int votes_for_new_game;    // NOWE: Liczba głosów za nową grą
    
    WorkerThread();
    ~WorkerThread();
};

#endif // HELPERS_H