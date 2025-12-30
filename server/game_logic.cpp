#include "game_logic.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <random>
#include <algorithm>

// Macierz w prostej postaci do edycji
std::vector<std::vector<char>> matrix_grid;

// Lista graczy
std::vector<Player> players;

// Dostępne kolory i ID
const std::vector<char> available_colors = {'R', 'B', 'G', 'Y'};
const std::vector<char> small_colors = {'r', 'b', 'g', 'y'};
std::vector<bool> color_used = {false, false, false, false};

// Implementacja konstruktora Player
Player::Player() : cfd(-1), row_position(-1), col_position(-1), 
                   color('\0'), player_id(-1), coloring(false), next_move('\0'), direction('u'), is_alive(true) { }

// Funkcja do tworzenia struktury gracza
Player create_player(int fd) {
    Player new_player;
    new_player.cfd = fd;
    new_player.coloring = false;
    
    // Znajdź pierwszy wolny kolor i przypisz odpowiednie id gracza
    for (size_t i = 0; i < available_colors.size(); ++i) {
        if (!color_used[i]) {
            new_player.color = available_colors[i];
            new_player.player_id = i + 1;
            color_used[i] = true;
            break;
        }
    }
    
    return new_player;
}

// Funkcja do zwalniania koloru gracza
void release_player_color(const Player& player) {
    if (player.player_id >= 1 && player.player_id <= 4) {
        color_used[player.player_id - 1] = false;
    }
}

// Funkcja przekształcająca macierz do ładnej postaci (string)
std::string matrix_to_string(const std::vector<std::vector<char>>& grid) {
    std::ostringstream matrix_stream;
    int n = grid.size();
    if (n == 0) return "";
    
    int m = grid[0].size();
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            matrix_stream << grid[i][j];
            if (j < m - 1) {
                matrix_stream << " ";
            }
        }
        matrix_stream << "\n";
    }
    
    return matrix_stream.str();
}

// Funkcja usuwająca gracza z macierzy
bool remove_player_from_grid(int player_id) {
    if (matrix_grid.empty()) return false;
    
    // Znajdź gracza w vectorze
    auto it = std::find_if(players.begin(), players.end(), 
                          [player_id](const Player& p) { return p.player_id == player_id; });
    
    if (it == players.end()) return false;
    
    char player_symbol = '0' + player_id;
    char player_color = it->color;
    
    // Usuwamy pola gracza z macierzy (zamieniamy na '0')
    int n = matrix_grid.size();
    int m = matrix_grid[0].size();
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            // Usuwamy symbol gracza
            if (matrix_grid[i][j] == player_symbol) {
                matrix_grid[i][j] = '0';
            }
            // Opcjonalnie: usuwamy też kolor gracza (jeśli chcesz)
            // else if (matrix_grid[i][j] == player_color) {
            //     matrix_grid[i][j] = '0';
            // }
        }
    }
    
    // Zwalniamy kolor gracza
    release_player_color(*it);
    
    // Usuwamy gracza z vectora
    players.erase(it);
    
    std::cout << "Gracz " << player_id << " zostal usuniety z gry" << std::endl;
    return true;
}

// Logika gry
void game_logic(std::shared_ptr<WorkerThread> worker) {
    std::cout << "Watek game_logic uruchomiony!" << std::endl;
    
    // Inicjalizujemy macierz 20x20 w prostej postaci
    matrix_grid.resize(GAME_GRID_SIZE, std::vector<char>(GAME_GRID_SIZE, '0'));
    //Tworzymy zmienna ktora mierzy czas
    auto start_time = std::chrono::steady_clock::now();
    
    while (worker->is_running) {
        // Sprawdzamy czy są komunikaty z serwera
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(worker->control_pipe_fd[0], &read_fds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; 
        
        int select_result = select(worker->control_pipe_fd[0] + 1, &read_fds, nullptr, nullptr, &tv);
        
        if (select_result > 0 && FD_ISSET(worker->control_pipe_fd[0], &read_fds)) {
            GameMessage msg;
            ssize_t n = read(worker->control_pipe_fd[0], &msg, sizeof(msg));
            
            if (n == sizeof(msg)) {
                switch (msg.type) {
                    case MessageType::NEW_PLAYER: {
                        std::cout << "Game Logic: Dodaje nowego gracza (fd=" << msg.client_fd << ")" << std::endl;
                        
                        // Tworzymy nowego gracza
                        Player new_player = create_player(msg.client_fd);
                        
                        if (new_player.player_id != -1) {
                            // Próbujemy umieścić gracza na planszy
                            int result = matrix_place_player(new_player); // Przekazujemy new_player przez referencję
                            
                            if (result == 1) {
                                players.push_back(new_player);
                                std::cout << "Gracz " << new_player.player_id 
                                         << " (kolor: " << new_player.color 
                                         << ") zostal dodany do gry!" << std::endl;
                            } else {
                                std::cout << "Nie udalo sie umiescic gracza na planszy!" << std::endl;
                                release_player_color(new_player);
                            }
                        } else {
                            std::cout << "Brak wolnych kolorow dla gracza!" << std::endl;
                        }
                        break;
                    }
                    
                    case MessageType::REMOVE_PLAYER: {
                        std::cout << "Game Logic: Usuwam gracza (fd=" << msg.client_fd << ")" << std::endl;
                        
                        // Znajdź gracza po fd
                        auto it = std::find_if(players.begin(), players.end(),
                                             [msg](const Player& p) { return p.cfd == msg.client_fd; });
                        
                        if (it != players.end()) {
                            int player_id = it->player_id;
                            remove_player_from_grid(player_id);
                        }
                        // sprawdzamy czy liczba graczy jest rowna 0. jesli jest to zamykamy caly watek
                        if (players.size() == 0) {
                            worker->is_running = false;
                            return;
                        }
                        break;
                        
                    }
                    case MessageType::PLAYER_MOVE: {
                        std::cout << "Game Logic: Otrzymano ruch gracza (fd=" << msg.client_fd << ", move=" << msg.move_data << ")" << std::endl;
                        
                        // Znajdź gracza po deskryptorze pliku (fd)
                        auto it = std::find_if(players.begin(), players.end(),
                                             [msg](const Player& p) { return p.cfd == msg.client_fd; });
                        
                        if (it != players.end()) {
                            it->next_move = msg.move_data; // Ustawiamy pole next_move gracza
                            std::cout << "Gracz " << it->player_id << " ustawil ruch na: " << it->next_move << std::endl;
                        }
                        break;
                    }

                    
                    default:
                        break;
                }
            }
        }
        
        // odswiezenie gry
        auto end_time = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
        if (ms >= GAME_REFRESH_INTERVAL){
            if (DEBUGGING)      std::cout<<"Czas interwalu: "<<ms<<std::endl;
            
            // zerujemy zegar
            start_time = std::chrono::steady_clock::now();
            
            // Wykonujemy ruchy wszystkich graczy
            for (auto& player : players) {
                if (player.is_alive) { // Wykonuj ruch tylko dla żyjących graczy
                    player_move(player);
                }
            }
            


            // Przekształcamy macierz do ładnej postaci
            std::string matrix = matrix_to_string(matrix_grid);
            
            // Wysyłamy rozmiar macierzy, a potem samą macierz przez pipe
            size_t matrix_size = matrix.size();
            
            // Najpierw wysyłamy rozmiar
            if (write(worker->game_pipe_fd[1], &matrix_size, sizeof(matrix_size)) == -1) {
                if (errno != EPIPE) {
                    perror("Blad wysylania rozmiaru macierzy przez game_pipe");
                }
                break;
            }
            
            // Potem wysyłamy samą macierz
            if (write(worker->game_pipe_fd[1], matrix.c_str(), matrix_size) == -1) {
                if (errno != EPIPE) {
                    perror("Blad wysylania macierzy przez game_pipe");
                }
                break;
            }
            
            std::cout << "Wyslano macierz " << GAME_GRID_SIZE << "x" << GAME_GRID_SIZE 
                    << " (" << matrix_size << " bajtow), liczba graczy: " << players.size() << std::endl;
            

        }
    }
    
    std::cout << "Watek game_logic zakonczony! - jaki smutek" << std::endl;
}

// Funkcja sprawdzająca czy kwadrat 3x3 jest wolny
bool is_3x3_free(const std::vector<std::vector<char>>& grid, int center_row, int center_col) {
    int n = grid.size();
    int m = grid[0].size();
    
    if (center_row < 1 || center_row >= n - 1 || center_col < 1 || center_col >= m - 1) {
        return false;
    }
    
    for (int i = center_row - 1; i <= center_row + 1; ++i) {
        for (int j = center_col - 1; j <= center_col + 1; ++j) {
            if (grid[i][j] != '0') {
                return false;
            }
        }
    }
    
    return true;
}

// Funkcja umieszczająca gracza w kwadracie 3x3
void place_player_in_3x3(std::vector<std::vector<char>>& grid, int center_row, 
                         int center_col, char kolor, int id_gracza) {
    for (int i = center_row - 1; i <= center_row + 1; ++i) {
        for (int j = center_col - 1; j <= center_col + 1; ++j) {
            grid[i][j] = kolor;
        }
    }
    
    grid[center_row][center_col] = '0' + id_gracza;
}

int matrix_place_player(Player& player) { // Zmieniona sygnatura
    int n = GAME_GRID_SIZE; // Używamy stałej globalnej
    int m = GAME_GRID_SIZE; // Używamy stałej globalnej
    if (matrix_grid.empty() || n < 3 || m < 3) {
        return -1;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_row(1, n - 2);
    std::uniform_int_distribution<> dis_col(1, m - 2);
    
    int random_row = dis_row(gen);
    int random_col = dis_col(gen);
    
    if (is_3x3_free(matrix_grid, random_row, random_col)) {
        place_player_in_3x3(matrix_grid, random_row, random_col, player.color, player.player_id);
        player.row_position = random_row; // Aktualizujemy pozycję bezpośrednio w obiekcie Player
        player.col_position = random_col;
        
        return 1;
    }
    
    for (int i = 1; i < n - 1; ++i) {
        for (int j = 1; j < m - 1; ++j) {
            if (is_3x3_free(matrix_grid, i, j)) {
                place_player_in_3x3(matrix_grid, i, j, player.color, player.player_id);
                player.row_position = i; // Aktualizujemy pozycję bezpośrednio w obiekcie Player
                player.col_position = j;
                
                return 1;
            }
        }
    }
    
    return -1;
}

/*Funkcja edytuje macierz tak aby gracz sie na niej poruszyl*/
void player_move(Player& player) {

    // ustalamy w ktora strone dany gracz bedzie sie poruszal
    char small_color = small_colors[player.player_id - 1];
    switch (player.direction)
    {
    case 'u': // przypadek gdy porusza sie do gory
        if (player.next_move == 'a') {
            player.direction = 'l';
        } else if (player.next_move == 'd') {
            player.direction = 'r';
        }
        break;
    case 'd': // przypadek gdy porusza sie w dol
        if (player.next_move == 'a') {
            player.direction = 'r';
        } else if (player.next_move == 'd') {
            player.direction = 'l';
        }
        break;
    case 'l': // przypadek gdy porusza sie w lewo
        if (player.next_move == 'a') {
            player.direction = 'd';
        } else if (player.next_move == 'd') {
            player.direction = 'u';
        }
        break;
    case 'r': // przypadek gdy porusza sie w prawo
        if (player.next_move == 'a') { //skret w lewo
            player.direction = 'u';
        } else if (player.next_move == 'd') { // skret w prawo
            player.direction = 'd';
        }
        break;
    default:
        break;
    }
    
    // Resetujemy next_move po przetworzeniu
    player.next_move = '\0'; // Zmieniono na '\0' dla poprawnego resetu

    // Obliczamy potencjalną następną pozycję
    int next_row = player.row_position;
    int next_col = player.col_position;

    switch (player.direction) {
        case 'u': next_row--; break;
        case 'd': next_row++; break;
        case 'l': next_col--; break;
        case 'r': next_col++; break;
    }

    // sprawdzamy kolizje z granicami 
    if (next_row < 0 || next_row >= GAME_GRID_SIZE || next_col < 0 || next_col >= GAME_GRID_SIZE) {
        std::cout << "Gracz " << player.player_id << " uderzyl w sciane! Ginie." << std::endl;
        player.is_alive = false;
        return;
    }

    
    // small_color jest już zadeklarowane na początku funkcji
    // char small_color = small_colors[player.player_id - 1]; 
    char target_cell = matrix_grid[next_row][next_col];

    if ((player.coloring && target_cell == small_colors[player.player_id - 1]) || 
        (target_cell != '0' && target_cell != player.color && target_cell != small_colors[player.player_id - 1])) {
        std::cout << "Gracz " << player.player_id << " zderzyl sie z " << target_cell << "! Ginie." << std::endl;
        player.is_alive = false;
        return;
    }

    // Wykonujemy ruch 
    // Jeśli gracz koloruje i wchodzi na swój własny kolor, zamyka pętlę.
    if (player.coloring && target_cell == player.color) {
        std::cout << "Gracz " << player.player_id << " zamknal petle!" << std::endl;
        // TODO: Tutaj zaimplementuj logikę wypełniania obszaru
        player.coloring = false;
        // Po wypełnieniu, obecna pozycja staje się kolorem gracza
        matrix_grid[player.row_position][player.col_position] = player.color;
    } 
    else if (target_cell == player.color) // jesli gracz poprostu chodzi po swoim terytorium to nic sie nie zmienia
    {
        //matrix_grid[next_row][next_col] = player.color;
        matrix_grid[player.row_position][player.col_position] = player.color;
    }
        
    else {
        // Jeśli nie koloruje, lub nie zamyka pętli
        if (player.coloring) {

            matrix_grid[player.row_position][player.col_position] = small_color;
        } else {
            // Rozpocznij kolorowanie, obecna pozycja staje się kolorem gracza
            player.coloring = true;
            player.matrix_before_coloring = matrix_grid; // Zapisz stan przed rozpoczęciem kolorowania
            matrix_grid[player.row_position][player.col_position] = player.color;
        }
    }

    // Aktualizujemy pozycję gracza
    player.row_position = next_row;
    player.col_position = next_col;

    // Umieszczamy gracza na nowej pozycji
    matrix_grid[player.row_position][player.col_position] = player.player_id + '0';
}
