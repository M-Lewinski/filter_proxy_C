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

void startProxyServer(char *port,struct configStruct* config);

#endif