#ifndef CLIENT_H
#define CLIENT_H
#include <string>
#include <QString>

using namespace std;

int connectToServer(const char* ip, int port);

void sendMove(int fd, const char* key);

void shut_conn(int fd);

QString statistics(vector<vector<string>> matrix, QString color);

#endif // CLIENT_H
