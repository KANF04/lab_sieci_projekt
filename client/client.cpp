#include "client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "gamewindow.h"
#include <iostream>
#include <sstream>
#include <vector>
#include "grid.h"
#include "dialog.h"
#include <QString>

using namespace std;

/*----------------------------------------------------FUNKCJE Logiczne----------------------------------------------------*/

// tworzy macierz
vector<vector<string>> mkMatrix(const vector<char> &matrixBuffer, ssize_t n) {

    vector<vector<string>> matrix;

    string line;


    string text(matrixBuffer.data(), n);

    stringstream all(text);

    while (getline(all, line)) {
        if(line.empty())
            continue;

        stringstream ls(line);
        string token;
        vector<string> row;

        while (ls >> token) {
            row.push_back(token);
        }

        matrix.push_back(row);
    }

    return matrix;
}

// oblicza procent zajętej mapy
QString statistics(vector<vector<string>> matrix, QString color) {
    float counter = 0.0f;

    int matrixSize = matrix.size() * matrix[0].size();

    for (int i=0; i<matrix.size(); i++) {
        for (int j=0; j<matrix[0].size(); j++) {
            if (color == "czerwony" ) {
                if (matrix[i][j] == "R") {
                    counter++;
                }
            }
            else if (color == "niebieski") {
                if (matrix[i][j] == "B") {
                    counter++;
                }
            }
            else if (color == "zielony") {
                if (matrix[i][j] == "G") {
                    counter++;
                }
            }
            else if (color == "żółty") {
                if (matrix[i][j] == "Y") {
                    counter++;
                }
            }
        }
    }
    float covPercentage = (counter / matrixSize) * 100;

    return QString::number(covPercentage);
}

/*----------------------------------------------------FUNKCJE SIECIOWE----------------------------------------------------*/

int connectToServer(const char* ip, int port) {

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error");
        return -1;
    }


    sockaddr_in serv_addr {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("bad ip");
        close(fd);
        return -1;
    }

    int ret = connect(fd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            perror("connection error");
            close(fd);
            return -1;
        }
    }

    return fd;
}

void sendMove(int fd, const char* key) {
    write(fd, key, strlen(key));
    write(1,key, strlen(key));
}

void shut_conn(int fd) {
    close(fd);
    shutdown(fd, SHUT_RDWR);
}

void printRecvMsg(int fd, GameWindow *window) {

    static vector<char> matrixBuffer(4096);

    ssize_t n = read(fd, matrixBuffer.data(), matrixBuffer.size());

    if (n <= 0)
        return;

    write(1, matrixBuffer.data(), n); // wyświetla macierz w terminalu
    static QString color;
    bool isDead = false;
    bool hasWon = false;
    bool hasLost = false;

    if (matrixBuffer[1] == '\n') {
        switch (matrixBuffer[0]) {
            case '1': color = "czerwony"; break;
            case '2': color = "niebieski"; break;
            case '3': color = "zielony"; break;
            case '4': color = "żółty"; break;
            case 'D': isDead = true; break;
            case 'W': hasWon = true; break;
            case 'L': hasLost = true; break;
            default: break;
        }

        matrixBuffer.erase(matrixBuffer.begin(), matrixBuffer.begin() + 2);

        n= n-2;

        vector<vector<string>> matrix = mkMatrix(matrixBuffer, n); // tworzy macierz z odczytanych danych w przypadku odczytania dodatkowych informacji od serwera
        window->setMatrix(matrix);

        window->setColor(color);
        if (isDead) {
            window->deadMessage();
            window->setIsDead(true);
        }

        QString info;

        if (hasWon || hasLost) {
            if (hasWon) {
                info = "WYGRAŁES!!!";
            } else {
                info = "PRZEGRAŁES :(";
            }

            QString statistic = statistics(matrix, color);
            Dialog *dialog = new Dialog(fd, info, statistic);
            dialog->show();
        }

    }
    else {
        vector<vector<string>> matrix = mkMatrix(matrixBuffer, n); // tworzy macierz w przypadku gdy serwer wysyla tylko macierz
        window->setMatrix(matrix);
    }

}
