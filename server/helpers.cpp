#include "helpers.h"
#include <unistd.h>
#include <cstdio>

WorkerThread::WorkerThread() : client_count(0), is_running(false), epoll_fd(-1),
    color_used(4, false), game_ended(false), votes_for_new_game(0) {  
    
    if (pipe(pipe_fd) == -1) {
        perror("Blad tworzenia pipe");
    }
    if (pipe(game_pipe_fd) == -1) {
        perror("Blad tworzenia game_pipe");
    }
    if (pipe(control_pipe_fd) == -1) {
        perror("Blad tworzenia control_pipe");
    }
    
    matrix_grid.resize(GAME_GRID_SIZE, std::vector<char>(GAME_GRID_SIZE, '0'));
    matrix_before_coloring = matrix_grid;
}

WorkerThread::~WorkerThread() {
    if (pipe_fd[0] != -1) close(pipe_fd[0]);
    if (pipe_fd[1] != -1) close(pipe_fd[1]);
    if (game_pipe_fd[0] != -1) close(game_pipe_fd[0]);
    if (game_pipe_fd[1] != -1) close(game_pipe_fd[1]);
    if (control_pipe_fd[0] != -1) close(control_pipe_fd[0]);
    if (control_pipe_fd[1] != -1) close(control_pipe_fd[1]);
    if (epoll_fd != -1) close(epoll_fd);
}