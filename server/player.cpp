#include "player.h"

// Implementacja konstruktora Player
Player::Player() : cfd(-1), row_position(-1), col_position(-1), 
                   color('\0'), player_id(-1), coloring(false), 
                   next_move('\0'), direction('u'), is_alive(true) { }