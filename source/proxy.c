#include <wchar.h>
#include "proxy.h"


int max_events = 512;
int connections = 0;
int maxConnections = 256;
int consoleDesc = 0;
int serverSoc = -1;
struct configStruct* configStructure=NULL;

int createAndListenServerSocket(char *port, char *address) {
    struct sockaddr_in serverSocAddr;
    if(atoi(port) <=0 || atoi(port) > 65535){
        fprintf(stderr, "Invalid port number");
        return -1;
    }
    serverSocAddr.sin_port = htons((uint16_t) atoi(port));
    serverSocAddr.sin_family = AF_INET;
    if(inet_aton(address, &serverSocAddr.sin_addr)==0){
        fprintf(stderr, "Invalid address");
        return -1;
    }

    int serverSoc = socket(AF_INET, SOCK_STREAM ,0);
    if(serverSoc==-1){
        fprintf(stderr,"Unable to create server socket");
        return -1;
    }
    int optval=1;
    setsockopt(serverSoc, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    if(bind(serverSoc, (const struct sockaddr*)&serverSocAddr, sizeof(serverSocAddr)) < 0){
        fprintf(stderr,"Failed to bind socket");
        return -1;
    }
    if(listen(serverSoc,SOMAXCONN)==-1){
        perror ("listen");
        return -1;
    }
    return serverSoc;
}

int handleConsoleConnection() {
    char buf[16];
    int j=0;
    for(j=0;j<16;j++) buf[j]=0;
    ssize_t readed = read(0, buf, 16);
    if(readed>=4 && !strcmp("exit\n",buf)) return 0;
    return 1;
}

int handleNewConnection(int serverFd, int epoolFd, struct requestStruct *pStruct) {
    struct epoll_event clientEvent;
    int clientLen = 0;
    struct sockaddr_in clientAddr;
    int clientFd = accept(serverFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    if(clientFd==-1){
        fprintf(stderr, "Failed to accept connection: ");
        perror("accept");
        return -1;
    }
    pStruct->clientSoc=clientFd;

    clientEvent.data.ptr=pStruct;
    clientEvent.events = EPOLLIN;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, pStruct->clientSoc, &clientEvent)==-1){
        perror ("epoll_ctl");
        close(pStruct->clientSoc);
        return -1;
    }

    return 0;
}

int handleRequest(struct requestStruct *reqStruct, int epoolFd) {
    reqStruct->clientRequest = newRequest();
    readData(reqStruct->clientRequest, reqStruct->clientSoc, reqStruct->time);

    if(checkBlocked(configStructure,reqStruct)){
        send(reqStruct->clientSoc,response403,strlen(response403),0);
        return -1;
    }

    filterRequest(configStructure,reqStruct);
    printf("%s\n",requestToString(*reqStruct->clientRequest, 0));

//    if((reqStruct->serverSoc= sendRequest(reqStruct, epoolFd)) < 0){
//        send(reqStruct->clientSoc,notImplemented,strlen(notImplemented),0);
//        return -1;
//    }
//    return 0;
    return -1;
}

int handleServerResponse(struct requestStruct *reqStruct) {
    reqStruct->serverResponse = newRequest();
    readData(reqStruct->serverResponse, reqStruct->serverSoc, reqStruct->time);
//    filterResponse(configStructure, reqStruct);

    char* req = requestToString(*reqStruct->serverResponse,1);
    printf("RESPONSE :\n%s\n",req);
    sendAll(reqStruct->clientSoc,req);
    return -1;
};

void startProxyServer(char *port, char*address, struct configStruct* config){
    configStructure=config;
    struct epoll_event *events = (struct epoll_event *)malloc(max_events * sizeof(struct epoll_event));
    struct requestStruct **requests = (struct requestStruct**)malloc(maxConnections * sizeof(struct requestStruct*));

    serverSoc = createAndListenServerSocket(port, address);
    if(serverSoc==-1) return;

    int epoolFd = epoll_create1(0);
    if (epoolFd == -1) {
        perror ("epoll_create");
        close(serverSoc);
        return;
    }

    struct epoll_event serverInEvent, consoleEvent;
    serverInEvent.data.ptr = &serverSoc;
    consoleEvent.data.ptr = &consoleDesc;
    serverInEvent.events = EPOLLIN;
    consoleEvent.events = EPOLLIN;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, serverSoc, &serverInEvent)==-1){
        perror ("epoll_ctl");
        close(serverSoc);
        close(epoolFd);
        return;
    }
    epoll_ctl(epoolFd, EPOLL_CTL_ADD, 0, &consoleEvent);

    int loop=1;
    while(loop){
        int n, i;
        n = epoll_wait(epoolFd, events, max_events, 5000);
        if(n>(int)(max_events*0.8)){
            max_events*=2;
            events = realloc(events,max_events*sizeof(struct epoll_event));
        }
        for(i=0;i<n;i++){

            if(consoleDesc == *(int*)events[i].data.ptr){ //Handle console input to stop server
                loop=handleConsoleConnection();
            } else if(serverSoc == *(int*)events[i].data.ptr){ //Incoming connection
                struct requestStruct* req = newRequestStruct();

                if(handleNewConnection(serverSoc, epoolFd, req) == 0) {
                    requests[connections] = req;
                    connections++;
                }
                else{
                    continue;
                }
                if(connections >= (maxConnections-5)){
                    maxConnections*=2;
                    requests = (struct requestStruct**)realloc(requests,maxConnections * sizeof(struct requestStruct*));
                }
            } else { //Other events - read data from client ar from server and send it to client.
                struct requestStruct *reqPtr = (struct requestStruct *)events[i].data.ptr;
                if(reqPtr->serverSoc != -1){
                    if(handleServerResponse(reqPtr)==-1) removeRequestStruct(*reqPtr, requests, &connections);
                } else if (reqPtr->clientSoc != -1){
                    if(handleRequest(reqPtr,epoolFd)==-1) removeRequestStruct(*reqPtr, requests, &connections);
                }
            }
        }
    }
    free(events);
    close(epoolFd);
    close(serverSoc);
}

int sendRequest(struct requestStruct *request, int epoolFd) {
    if(request->serverSoc < 0){
        int newServerSocket;
        if( (newServerSocket = socket(AF_INET,SOCK_STREAM,0)) == -1){
            fprintf(stderr,"SERVER SOCKET ERROR\n");
            return -1;
        }
        struct addrinfo * serverInfo, hints;
        memset(&hints,0,sizeof hints);
        hints.ai_flags=0;
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_STREAM;
        int result;
        char *hostName = getHost(request->clientRequest);
        if(hostName == NULL){
            close(newServerSocket);
            return -1;
        }
        printf("HOST: %s\n",hostName);
        if ((result = getaddrinfo(hostName,"http",&hints,&serverInfo))){
            close(newServerSocket);
            fprintf(stderr,"GETADDRINFO ERROR\n");
            if(result == EAI_SYSTEM){
                fprintf(stderr,"GETADDRINFO: %s\n",strerror(errno));
            } else{
                fprintf(stderr,"GETADDRINFO: %s\n",gai_strerror(result));
            }
            return -1;
        }
        if(connect(newServerSocket,serverInfo->ai_addr,serverInfo->ai_addrlen)){
            freeaddrinfo(serverInfo);
            close(newServerSocket);
            fprintf(stderr,"CONNECTION ERROR\n");
            return -1;
        }
        freeaddrinfo(serverInfo);
        printf("NEW SERVER: %d\n",newServerSocket);

        request->serverSoc = newServerSocket;
        struct epoll_event serverEvent;
        serverEvent.data.ptr=request;
        serverEvent.events = EPOLLIN | EPOLLOUT;
        if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, request->serverSoc, &serverEvent)==-1){
            perror ("epoll_ctl");
            close(newServerSocket);
            request->serverSoc = -1;
            return -1;
        }
    }
    char* req = requestToString(*request->clientRequest,0);
    printf("REQUEST :\n%s\n",req);
    if(sendAll(request->serverSoc,req) < 0){
        return -1;
    }
    free(req);
    return request->serverSoc;
}

int sendAll(int socket,char *text){
    if (strlen(text) == 0){
        fprintf(stderr,"EMPTY BUFFER\n");
        return -1;
    }
    char* buffer = text;
    size_t i = strlen(text);
    while(i > 0){
        ssize_t sentBytes = send(socket,buffer,i,0);
        if (sentBytes < 0){
            fprintf(stderr,"ERROR SEND: %s\n",strerror(errno));
            return -1;
        }
        buffer += sentBytes;
        i -= sentBytes;
    }
    return 1;
}