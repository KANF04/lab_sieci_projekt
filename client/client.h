#ifndef CLIENT_H
#define CLIENT_H
#include <string>

using namespace std;

int connectToServer(const char* ip, int port);

void sendMove(int fd, const char* key);

void shut_conn(int fd);

#endif // CLIENT_H
