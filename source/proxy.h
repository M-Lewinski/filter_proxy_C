#ifndef HOST_H
#define HOST_H

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "responseStatus.h"

struct requestStruct{
    int clientSoc;
    int serverSoc;
    time_t time;
};

/**
 * Create server socket for given port and return it
 * @param port used in binding server socket
 * @return server socket handler
 */
int createAndListenServerSocket(char *port);

/**
 * Read from standard input and check if value is `exit`. It that's true return 0;
 * @return 0 if server should stop 1 in other case
 */
int handleConsoleConnection();

/**
 * Create new connection witch client and add it to epoll events
 * @param serverFd server socket
 * @param epoolFd epoll handler
 */
void handleNewConnection(int serverFd, int epoolFd);

/**
 * Should handle incoming request: read data, parse and filter request and make call to server;
 * @param clientFd
 */
void handleRequest(int clientFd);

/**
 * Start proxy server
 * @param port port used in binding server socket
 * @param config with filter rules
 */
void startProxyServer(char *port,struct configStruct* config);

#endif