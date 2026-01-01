#ifndef PLAYER_H
#define PLAYER_H

#include <vector>

struct Player {
    int cfd;
    int row_position;
    int col_position;
    char color;
    int player_id;
    bool coloring;
    char next_move;
    bool is_alive;
    char direction;
    
    Player();
};

#endif // PLAYER_H