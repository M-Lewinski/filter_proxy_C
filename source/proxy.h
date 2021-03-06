#ifndef HOST_H
#define HOST_H

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "responseStatus.h"
#include <time.h>
#include "rule.h"
#include <stddef.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

struct threadParametrs{
    pthread_mutex_t *requestMutex;
    pthread_cond_t *endCondition;
    struct requestStruct ***requests;
    struct requestStruct *req;
    int *connections;
    int *threadCount;
    int *threadAlive;
    int epollFd;
    pthread_t  *threadId;
};


struct threadParametrs *newThread(int epollFd, pthread_mutex_t *mutex, struct requestStruct *req, struct requestStruct ***requests,
                                  int *connections, int *threadCount, int *threadAlive, pthread_t *threadId, pthread_cond_t *endCondition);


void freethreadParametrs(struct threadParametrs* param);

void * threadHandleNewConnection(void *thread);

void *threadHandleServerResponse(void *thread);

void *threadHandleClientRequest(void *thread);

/**
 * Create server socket for given port and return it
 * @param port used in binding server socket
 * @return server socket handler
 */
int createAndListenServerSocket(char *port, char *address);

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
int handleNewConnection(int serverFd, int epoolFd, struct requestStruct *pStruct);

/**
 * Should handle incoming request: read data, parse and filter request and make call to server;
 * @param clientFd
 * @return -1 if we should remove response and close socket, 0 in other case
 */
int handleRequest(struct requestStruct *reqStruct, int epoolFd, int *threadAlive);

/**
 * Handle server response and write data to client socket
 * @param reqStruct struct with client and server sockets
 * @return -1 if we should remove response and close socket, 0 in other case
 */
int handleServerResponse(struct requestStruct *reqStruct, int *threadAlive);

/**
 * Start proxy server
 * @param port port used in binding server socket
 * @param config with filter rules
 */
void startProxyServer(char *port, char*address, struct configStruct* config);

/**
 * Send request to server. Request is created from headers
 * @param request pointer to request struct
 * @return new server socket or -1 if error occured
 */
int sendRequest(struct requestStruct *request, int epoolFd, int *threadAlive);

/**
 *
 * @param socket socket to which message will be send
 * @param text message which will be send
 * @return positive if succesfull, negative if failed
 */
int sendAll(int socket, char *text, int size);




void freethreadParametrs(struct threadParametrs* param);

#endif