#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include "player.h"  
#include "helpers.h"

// Deklaracje funkcji
void game_logic(std::shared_ptr<WorkerThread> worker);
std::string matrix_to_string(const std::vector<std::vector<char>>& grid);
int matrix_place_player(Player& player, std::shared_ptr<WorkerThread> worker);
Player create_player(int fd, std::shared_ptr<WorkerThread> worker);
void release_player_color(const Player& player, std::shared_ptr<WorkerThread> worker);
void player_move(Player& player, std::shared_ptr<WorkerThread> worker); 
bool remove_player_from_grid(int player_id, std::shared_ptr<WorkerThread> worker, bool death); 
std::vector<Player>::iterator find_player_on_id(int player_id, std::shared_ptr<WorkerThread> worker); 
void fill_closed_area(int player_id, std::shared_ptr<WorkerThread> worker);

// Zmienne globalne 
extern const std::vector<char> available_colors;
extern const std::vector<char> small_colors;

#endif // GAME_LOGIC_H