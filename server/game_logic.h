#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include "helpers.h"

// Deklaracja struktury Player
struct Player {
    int cfd;    //clients file describer
    int row_position;   //poztcja gracza rzad
    int col_position;   // pozycja gracza kolumna
    char color; // kolor [r,g,b,y]
    int player_id;    // id gracza [1,2,3,4]
    bool coloring;  // czy w trybie kolorowania - domyslnie false
    char next_move;  // czy zmienia kierunek poruszania sie 
    bool is_alive;   // Czy gracz żyje
    char direction; // w ktorym kierunku sie porusza
    std::vector<std::vector<char>> matrix_before_coloring; // celem tej macierzy jest przy smierci aby wszystkie pola w czasie kolorowania byly zwrocone do wlasciwej formy
    Player();
};

// Deklaracje funkcji
void game_logic(std::shared_ptr<WorkerThread> worker);
std::string matrix_to_string(const std::vector<std::vector<char>>& grid);
int matrix_place_player(Player& player); // Zmieniona sygnatura
Player create_player(int fd);
void release_player_color(const Player& player);
void player_move(Player& player); 
bool remove_player_from_grid(int player_id);

// Zmienne globalne
extern std::vector<std::vector<char>> matrix_grid;
extern std::vector<Player> players;
extern std::vector<bool> color_used;
extern const std::vector<char> available_colors;
extern const std::vector<char> small_colors;

#endif // GAME_LOGIC_H