#include "game_logic.h"
#include "player.h" 
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <random>
#include <algorithm>
#include <queue> // Dodano dla BFS w funkcji fill_closed_area

// Tylko stałe pozostają globalne
const std::vector<char> available_colors = {'R', 'B', 'G', 'Y'};
const std::vector<char> small_colors = {'r', 'b', 'g', 'y'};

// Funkcja do tworzenia struktury gracza
Player create_player(int fd, std::shared_ptr<WorkerThread> worker) {
    Player new_player;
    new_player.cfd = fd;
    new_player.coloring = false;
    
    // Znajdź pierwszy wolny kolor i przypisz odpowiednie id gracza
    for (size_t i = 0; i < available_colors.size(); ++i) {
        if (!worker->color_used[i]) {
            new_player.color = available_colors[i];
            new_player.player_id = i + 1;
            worker->color_used[i] = true;
            break;
        }
    }
    
    return new_player;
}

// Funkcja do zwalniania koloru gracza
void release_player_color(const Player& player, std::shared_ptr<WorkerThread> worker) {
    if (player.player_id >= 1 && player.player_id <= 4) {
        worker->color_used[player.player_id - 1] = false;
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
// Obsluguje rowniez smierc gracza
bool remove_player_from_grid(int player_id, std::shared_ptr<WorkerThread> worker, bool death) {
    if (worker->matrix_grid.empty()) return false;
    
    // Znajdź gracza w vectorze
    auto it = find_player_on_id(player_id, worker);
    if (it == worker->players.end()) { // Sprawdzamy, czy gracz został znaleziony
        return false; // Jeśli nie znaleziono, zwracamy false
    }
    
    char player_symbol = '0' + player_id;
    char player_color = it->color; // Dostęp do pól gracza przez iterator
    char small_player_color = small_colors[player_id - 1];
    
    // Usuwamy pola gracza z macierzy (zamieniamy na '0')
    int n = GAME_GRID_SIZE; // Używamy stałej GAME_GRID_SIZE
    int m = GAME_GRID_SIZE; // Używamy stałej GAME_GRID_SIZE
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            // Usuwamy symbol gracza
            if (worker->matrix_grid[i][j] == player_symbol) {
                worker->matrix_grid[i][j] = '0';
            }
            // Usuwamy kolor gracza
            else if (worker->matrix_grid[i][j] == player_color) {
                worker->matrix_grid[i][j] = '0';
                worker->matrix_before_coloring[i][j] = '0';
            // usuwamy tryb kolorowania gracza
            } else if (worker->matrix_grid[i][j] == small_player_color) {
                worker->matrix_grid[i][j] = worker->matrix_before_coloring[i][j];
            }
        }
    }
    
    // Zwalniamy kolor gracza
    if (!death){
        release_player_color(*it, worker); 
    
        // Usuwamy gracza z vectora
        worker->players.erase(it);
        std::cout << "Gracz " << player_id << " zostal usuniety z gry" << std::endl;
        return true;    
    }
    if (DEBUGGING) std::cout << "Gracz " << player_id << " zostal zabity" << std::endl;

    it->is_alive = false; 

    
    
    
    return true;
        
}

// Logika gry
void game_logic(std::shared_ptr<WorkerThread> worker) {
    std::cout << "Watek game_logic uruchomiony!" << std::endl;
    
    // Macierz jest już zainicjalizowana w konstruktorze WorkerThread
    // Tworzymy zmienną która mierzy czas
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
                        Player new_player = create_player(msg.client_fd, worker);
                        
                        if (new_player.player_id != -1) {
                            // Próbujemy umieścić gracza na planszy
                            int result = matrix_place_player(new_player, worker);
                            
                            if (result == 1) {
                                worker->players.push_back(new_player);
                                std::cout << "Gracz " << new_player.player_id 
                                         << " (kolor: " << new_player.color 
                                         << ") zostal dodany do gry!" << std::endl;
                            } else {
                                std::cout << "Nie udalo sie umiescic gracza na planszy!" << std::endl;
                                release_player_color(new_player, worker);
                            }
                        } else {
                            std::cout << "Brak wolnych kolorow dla gracza!" << std::endl;
                        }
                        break;
                    }
                    
                    case MessageType::REMOVE_PLAYER: {
                        std::cout << "Game Logic: Usuwam gracza (fd=" << msg.client_fd << ")" << std::endl;
                        
                        // Znajdź gracza po fd
                        auto it = std::find_if(worker->players.begin(), worker->players.end(),
                                             [msg](const Player& p) { return p.cfd == msg.client_fd; });
                        
                        if (it != worker->players.end()) {
                            int player_id = it->player_id;
                            remove_player_from_grid(player_id, worker, false);
                        }
                        // sprawdzamy czy liczba graczy jest rowna 0. jesli jest to zamykamy caly watek
                        if (worker->players.size() == 0) {
                            worker->is_running = false;
                            return;
                        }
                        break;
                        
                    }
                    case MessageType::PLAYER_MOVE: {
                        std::cout << "Game Logic: Otrzymano ruch gracza (fd=" << msg.client_fd << ", move=" << msg.move_data << ")" << std::endl;
                        
                        // Znajdź gracza po deskryptorze pliku (fd)
                        auto it = std::find_if(worker->players.begin(), worker->players.end(),
                                             [msg](const Player& p) { return p.cfd == msg.client_fd; });
                        
                        if (it != worker->players.end()) {
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
            for (auto& player : worker->players) {
                if (player.is_alive) { // Wykonuj ruch tylko dla żyjących graczy
                    player_move(player, worker);
                }
            }
            
            // Przekształcamy macierz do ładnej postaci
            std::string matrix = matrix_to_string(worker->matrix_grid);
            
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
                    << " (" << matrix_size << " bajtow), liczba graczy: " << worker->players.size() << std::endl;
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
void place_player_in_3x3(std::shared_ptr<WorkerThread> worker, int center_row, 
                         int center_col, char kolor, int id_gracza) {
    for (int i = center_row - 1; i <= center_row + 1; ++i) {
        for (int j = center_col - 1; j <= center_col + 1; ++j) {
            worker->matrix_grid[i][j] = kolor;
            worker->matrix_before_coloring[i][j] = kolor;
        }
    }
    
    worker->matrix_grid[center_row][center_col] = '0' + id_gracza;
}

int matrix_place_player(Player& player, std::shared_ptr<WorkerThread> worker) {
    int n = GAME_GRID_SIZE;
    int m = GAME_GRID_SIZE;
    if (worker->matrix_grid.empty() || n < 3 || m < 3) {
        return -1;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_row(1, n - 2);
    std::uniform_int_distribution<> dis_col(1, m - 2);
    
    int random_row = dis_row(gen);
    int random_col = dis_col(gen);
    
    if (is_3x3_free(worker->matrix_grid, random_row, random_col)) {
        place_player_in_3x3(worker, random_row, random_col, player.color, player.player_id);
        player.row_position = random_row;
        player.col_position = random_col;
        
        return 1;
    }
    
    for (int i = 1; i < n - 1; ++i) {
        for (int j = 1; j < m - 1; ++j) {
            if (is_3x3_free(worker->matrix_grid, i, j)) {
                place_player_in_3x3(worker, i, j, player.color, player.player_id);
                player.row_position = i;
                player.col_position = j;
                
                return 1;
            }
        }
    }
    
    return -1;
}

/*Funkcja edytuje macierz tak aby gracz sie na niej poruszyl*/
void player_move(Player& player, std::shared_ptr<WorkerThread> worker) {

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
    
    // Zapamiętujemy aktualną pozycję gracza przed jej aktualizacją
    int old_player_row = player.row_position;
    int old_player_col = player.col_position;

    // Resetujemy next_move po przetworzeniu
    player.next_move = '\0';

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
        remove_player_from_grid(player.player_id, worker, true);
        return;
    }

    char target_cell = worker->matrix_grid[next_row][next_col];
    // zabijanie sie jesli samemu wjechalo sie w traase kolorowania
    if (target_cell == small_color) {
        if (DEBUGGING) {std::cout << "Gracz " << player.player_id << " zderzyl sie z  wlasnym kolorowaniem! Ginie." << std::endl;
            std::cout<< "Zderzyl sie z " << target_cell << ". Jego kolor to " << player.color << std::endl;
        }
        remove_player_from_grid(player.player_id, worker, true); // obsluga smierci przy zderzeniu sie z wlasnym szlaczkiem
        return;
    }
    // zabijanie sie jesli dwoch graczy wlecialo w siebie
    if (target_cell == '1' || target_cell == '2' || target_cell == '3' || target_cell == '4') {
        if (DEBUGGING) std::cout << "Gracz " << player.player_id << " zderzyl sie z innym graczem! Oboje gina." << std::endl;
        for (auto& other_player : worker->players) {
            if (other_player.player_id == target_cell - '0') {
                remove_player_from_grid(other_player.player_id, worker, true);
                
            }
        }
        remove_player_from_grid(player.player_id, worker, true);
        return;
    }

    // zabijanie gracza jesli wjechano w jego slad kolorowania
    if (target_cell == 'r' || target_cell == 'b' || target_cell == 'g' || target_cell == 'y') {
        
        //szukamy id gracza po kolorze
        for (int i = 0; i < worker->players.size(); ++i) {
            if (small_colors[i] == target_cell) {
                int enemy_id = i + 1;
                for (auto& other_player : worker->players) {
                    if (other_player.player_id == enemy_id) {
                        remove_player_from_grid(other_player.player_id, worker, true);
                        break;
                    }
                }
                break;
            }
        }
    }

    // Wykonujemy ruch 
    // Jeśli gracz koloruje i wchodzi na swój własny kolor, zamyka pętlę.
    if (player.coloring && target_cell == player.color) {
        std::cout << "Gracz " << player.player_id << " zamknal petle!" << std::endl;
        fill_closed_area(player.player_id, worker);
        player.coloring = false;
        // Po wypełnieniu, obecna pozycja staje się kolorem gracza
        worker->matrix_grid[player.row_position][player.col_position] = player.color;
    } 
    else if (target_cell == player.color) // jesli gracz poprostu chodzi po swoim terytorium to nic sie nie zmienia
    {
        worker->matrix_grid[player.row_position][player.col_position] = player.color;
    }
        
    else {
        // Jeśli nie koloruje, lub nie zamyka pętli
        if (player.coloring) {
            worker->matrix_grid[player.row_position][player.col_position] = small_color;
        } else {
            // Rozpocznij kolorowanie, obecna pozycja staje się kolorem gracza
            player.coloring = true;
            worker->matrix_grid[player.row_position][player.col_position] = player.color;
        }
    }

    // Aktualizujemy pozycję gracza
    player.row_position = next_row;
    player.col_position = next_col;

    // Umieszczamy gracza na nowej pozycji
    worker->matrix_grid[player.row_position][player.col_position] = player.player_id + '0';
}

// Funkcja szukajaca gracza
std::vector<Player>::iterator find_player_on_id(int player_id, std::shared_ptr<WorkerThread> worker) {
    for (auto it = worker->players.begin(); it != worker->players.end(); ++it) {
        if (it->player_id == player_id) {
            return it; // Zwracamy iterator
        }
    }
    return worker->players.end(); // Zwracamy end() jeśli nie znaleziono
}

/*
Funkcja ktora bedzie wypelniala figure zamknieta danym kolorem wskazanym przez player_id 
*/
void fill_closed_area(int player_id, std::shared_ptr<WorkerThread> worker) {
    // Szukamy gracza o podanym player_id
    auto it = find_player_on_id(player_id, worker);
    if (it == worker->players.end()) return; 
    
    char color = it->color;
    char small_color = small_colors[player_id - 1];
    
    int n = GAME_GRID_SIZE;
    int m = GAME_GRID_SIZE;
    
    // Najpierw zamieniamy wszystkie małe kolory na duże (zamykamy pętlę)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (worker->matrix_grid[i][j] == small_color) {
                worker->matrix_grid[i][j] = color;
                worker->matrix_before_coloring[i][j] = color;
            }
        }
    }
    
    // Teraz oznaczamy granice mapy jako poza figura
    std::vector<std::vector<bool>> outside(n, std::vector<bool>(m, false));
    std::queue<std::pair<int, int>> q;
    
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if ((i == 0 || i == n-1 || j == 0 || j == m-1) && 
                worker->matrix_grid[i][j] != color) {
                q.push({i, j});
                outside[i][j] = true;
            }
        }
    }
    
   
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    
    // Teraz to co robimy to 'zalewamy'/oznaczamy wszystko co ma sasiadow 0 i nie jest R - mozemy poruszac sie tylko w gore prawo dol lewo
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        
        for (int dir = 0; dir < 4; ++dir) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];
            
            if (nx >= 0 && nx < n && ny >= 0 && ny < m && 
                !outside[nx][ny] && worker->matrix_grid[nx][ny] != color) {
                outside[nx][ny] = true;
                q.push({nx, ny});
            }
        }
    }
    
    // Wszystkie pola '0' które NIE są oznaczone jako outside są wewnątrz figury
    // Wiec je kolorujemy
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (worker->matrix_grid[i][j] == '0' && !outside[i][j]) {
                worker->matrix_grid[i][j] = color;
                worker->matrix_before_coloring[i][j] = color;
            }
        }
    }
    
    if (DEBUGGING) {
        std::cout << "Wypelniono zamkniety obszar gracza " << player_id 
                  << " kolorem " << color << std::endl;
    }
}