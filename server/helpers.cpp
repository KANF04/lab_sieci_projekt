#include "helpers.h"
#include <unistd.h> // For close()
#include <cstdio>   // For perror()

// Implementacja konstruktora WorkerThread
WorkerThread::WorkerThread() : client_count(0), is_running(false), epoll_fd(-1),
                                color_used(4, false) {  // Inicjalizuj color_used
    if (pipe(pipe_fd) == -1) {
        perror("Blad tworzenia pipe");
    }
    if (pipe(game_pipe_fd) == -1) {
        perror("Blad tworzenia game_pipe");
    }
    if (pipe(control_pipe_fd) == -1) {
        perror("Blad tworzenia control_pipe");
    }
    
    // Inicjalizuj macierz gry
    matrix_grid.resize(GAME_GRID_SIZE, std::vector<char>(GAME_GRID_SIZE, '0'));
}

// Implementacja destruktora WorkerThread
WorkerThread::~WorkerThread() {
    if (pipe_fd[0] != -1) close(pipe_fd[0]);
    if (pipe_fd[1] != -1) close(pipe_fd[1]);
    if (game_pipe_fd[0] != -1) close(game_pipe_fd[0]);
    if (game_pipe_fd[1] != -1) close(game_pipe_fd[1]);
    if (control_pipe_fd[0] != -1) close(control_pipe_fd[0]);
    if (control_pipe_fd[1] != -1) close(control_pipe_fd[1]);
    if (epoll_fd != -1) close(epoll_fd);
}