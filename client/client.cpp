#include "client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "gamewindow.h"
#include <iostream>

using namespace std;

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
}

void shut_conn(int fd) {
    close(fd);
    shutdown(fd, SHUT_RDWR);
}

void printRecvMsg(int fd) {
    static bool readingMatrix = false;
    static vector<char> matrixBuffer;


    ssize_t n = read(fd, &matrixBuffer, sizeof(matrixBuffer));
    if (n > 0) {
        write(1, &matrixBuffer, sizeof(matrixBuffer));
    }
}

