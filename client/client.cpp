#include "client.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

bool connectToServer(const char* ip, int port) {

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error");
        return false;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(fd, (sockaddr*) &addr, sizeof(addr));

    sockaddr_in serv_addr {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(fd, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection error");
        return false;
    }

    close(fd);
    shutdown(fd, SHUT_RDWR);

    return true;
}


