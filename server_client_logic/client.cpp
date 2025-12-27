#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include <poll.h>
#include <sys/epoll.h>
#include <thread>

#define BUFFSIZE 255



template <typename... Args> /* This mimics GNU 'error' function */
/*Nie wiem co to jeszcze robi ;) - jak ogarne napisze do tego komentarz*/
void error(int status, int errnum, const char *format, Args... args) {
    fprintf(stderr, (format + std::string(errnum ? ": %s\n" : "\n")).c_str(), args..., strerror(errnum));
    if (status) exit(status);
}

/*Funkcja readData ma na celu odczytanie danych w tcp z spojrzeniem na bledy. Wzorowana na tcp_client_template

args:

    - int fd - deskryptor pliku
    - char *buffer - bufor, do którego zostaną zapisane odczytane dane
    - ssize_t buffsize - maksymalna liczba bajtów do odczytania

return:
    - ssize_t - liczba odczytanych bajtów
*/
ssize_t readData(int fd, char *buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if (ret == -1) error(1, errno, "read failed on descriptor %d", fd);
    if (ret == 0) exit(0); 
    return ret;
}

/*Funkcja writeData ma na celu wysłanie danych do gniazda z obsługą błędów. Wzorowane na tcp_client_template

args:
    - int fd - deskryptor pliku
    - char *buffer - bufor z danymi do wysłania
    - ssize_t count - liczba bajtów do wysłania
*/
void writeData(int fd, char *buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if (ret == -1) error(1, errno, "write failed on descriptor %d", fd);
    if (ret != count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

int main(int argc, char **argv) {
    if (argc != 3) error(1, 0, "Usage: %s <host> <port>", argv[0]);

    // Resolve arguments to IPv4 address with a port number
    addrinfo *resolved, hints {};
	hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;

    // Error controll
    int res = getaddrinfo(argv[1], argv[2], &hints, &resolved);
    if (res) error(1, 0, "getaddrinfo: %s", gai_strerror(res));

    // create socket
    int sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
    if (sock == -1) error(1, errno, "socket failed");

    // attept to connect
    res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
    if (res) error(1, errno, "connect failed");

    // free memory
    freeaddrinfo(resolved);

    // Tutaj zrobimy dzialanie klienta

    // /****************************/

    // // read from socket, write to stdout
    // ssize_t received1;
    // char buffer1[BUFFSIZE];
    // received1 = readData(sock, buffer1, BUFFSIZE);
    // writeData(1, buffer1, received1);

    // /****************************/

    // // read from stdin, write to socket
    // ssize_t received2;
    // char buffer2[BUFFSIZE];
    // received2 = readData(0, buffer2, BUFFSIZE);
    // writeData(sock, buffer2, received2);

    // /****************************/

    shutdown(sock, SHUT_RDWR);
    close(sock);

    return 0;
}
