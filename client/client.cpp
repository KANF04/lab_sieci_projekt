#include "client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "gamewindow.h"

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

int sendMove(int fd, const char* key) {
     int msg = write(fd, key, strlen(key));
     return msg;
}

void shut_conn(int fd) {
    close(fd);
    shutdown(fd, SHUT_RDWR);
}


