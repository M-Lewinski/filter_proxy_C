#include "proxy.h"

char *localhost = (char*)"127.0.0.1";
int max_events = 256;
int connections = 0;
int maxConnections = 256;

struct requestStruct * newRequestStruct(){
    struct requestStruct *req = (struct requestStruct*)malloc(sizeof(struct requestStruct));
    req->clientSoc = -1;
    req->serverSoc = -1;
    req->request = NULL;
    req->time = time(0);
    return req;
}

void deleteRequestStruct (int requestIndex, struct requestStruct** requests){
    struct requestStruct *req = requests[requestIndex];
    if(req->clientSoc!=-1)close(req->clientSoc);
    if(req->serverSoc!=-1)close(req->serverSoc);
    free(req->request);
    free(req);
    if(requestIndex==connections-1) connections--;
    else requests[requestIndex] = requests[--connections];
}

int createAndListenServerSocket(char *port) {
    struct sockaddr_in serverSocAddr;
    serverSocAddr.sin_port = htons((uint16_t) atoi(port));
    serverSocAddr.sin_family = AF_INET;
    inet_aton(localhost, &serverSocAddr.sin_addr);

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
    char buf[4];
    for(int j=0;j<4;j++) buf[j]=0;
    ssize_t readed = read(0, buf, 4);
    if((readed==4) && (!strcmp("exit",buf))) return 0;
    return 1;
}

struct requestStruct * handleNewConnection(int serverFd, int epoolFd) {
    struct epoll_event clientEvent;
    int clientLen;
    struct sockaddr_in clientAddr;
    int clientFd = accept(serverFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    if(clientFd==-1){
        perror("accept");
        fprintf(stderr, "Failed to accept connection");
        return NULL;
    }
    clientEvent.data.fd=clientFd;
    clientEvent.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, clientFd, &clientEvent)==-1){
        perror ("epoll_ctl");
        return NULL;
    }

    struct requestStruct *req = newRequestStruct();
    req->clientSoc=clientFd;

    return req;
}

char* readData(char *data, int socket){
    //TODO CHANGE IMPLEMENTATION
    data=(char*)malloc(3000*sizeof(char));
    char buf[3000];
    int j=0;
    for (j=0;j<3000;j++)buf[j]='\0';

    ssize_t read;
    if( (read=recv(socket, buf, 3000, 0)) > 0) {
        //TODO read headers
        //TODO read form data
        //TODO how to know if end of reading?
        //TODO between headers and data we have empty line which means two \n \n characters
        //TODO in headers we have header with content length. It should be enough to read all data.
    }
    strcpy(data,buf);
    return data;
}

int handleRequest(struct requestStruct *reqStruct) {
    reqStruct->request = readData(reqStruct->request, reqStruct->clientSoc);

    //TODO REMOVE THIS WRITE
    write(1, reqStruct->request, strlen(reqStruct->request));


    //TODO CHECK BLOCK FILTER
    //TODO IF PAGE IS BLOCKED WE DO NOT NEED TO CALL SERVER. WE SHOULD RETURN ONLY response403, cloce socket and free memory

    //TODO MODIFY REQUEST USING FILTERS

    //ONLY FOR TEST
    //TODO REMOVE IT AND CREATE FUNCTION TO MAKE CALL TO SERVER
    send(reqStruct->clientSoc,response403,strlen(response403),0);
    return -1;
}

int handleServerResponse(struct requestStruct *reqStruct) {
    //TODO READ DATA FROM SERVER

    //TODO FILTER IF WE WANT TO IMPLEMENT FILTERING ON THIS LEVEL

    //TODO WRITE RESPONSE TO CLIENT
    //Hmnn, it should always return -1
    return -1;
};

void handleTimeout(struct requestStruct **requests) {
    //TODO REMOVE ALL OLD REQUESTS WITH TIMEOUT AND SEND MESSAGE WITH 504 status to client
}

void startProxyServer(char *port,struct configStruct* config){
    struct epoll_event *events = (struct epoll_event *)malloc(max_events * sizeof(struct epoll_event));
    struct requestStruct **requests = (struct requestStruct**)malloc(maxConnections * sizeof(struct requestStruct*));

    int serverSoc = createAndListenServerSocket(port);
    if(serverSoc==-1) return;

    int epoolFd = epoll_create1(0);
    if (epoolFd == -1) {
        perror ("epoll_create");
        return;
    }

    struct epoll_event serverInEvent, consoleEvent;
    serverInEvent.data.fd = serverSoc;
    consoleEvent.data.fd = 0;
    serverInEvent.events = EPOLLIN | EPOLLET;
    consoleEvent.events = EPOLLIN;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, serverSoc, &serverInEvent)==-1 || epoll_ctl(epoolFd, EPOLL_CTL_ADD, 0, &consoleEvent)==-1){
        perror ("epoll_ctl");
        return;
    }
    int loop=1;
    while(loop){
        int n, i, k;
        n = epoll_wait(epoolFd, events, 100, 5000);
        if(n>(int)(max_events*0.8)){
            max_events*=2;
            events = realloc(events,max_events*sizeof(struct epoll_event));
        }
        for(i=0;i<n;i++){
            if(0 == events[i].data.fd){ //Handle console input to stop server
                loop=handleConsoleConnection();
            } else if(serverSoc == events[i].data.fd){ //Incoming connection
                struct requestStruct* req = handleNewConnection(serverSoc, epoolFd);
                if(req != NULL) {
                    requests[connections] = req;
                    connections++;
                }
                if(connections >= (maxConnections-5)){
                    maxConnections*=2;
                    requests = realloc(requests,maxConnections * sizeof(struct requestStruct*));
                }
            } else { //Other events - read data and do sth.
                for (k = 0; k < connections; k++){
                    if (requests[k]->clientSoc != -1 && requests[k]->clientSoc == events[i].data.fd) {
                        if(handleRequest(requests[k])==-1) deleteRequestStruct(k,requests);
                        break;
                    }
                    if (requests[k]->serverSoc != -1 && requests[k]->serverSoc == events[i].data.fd) {
                        if(handleServerResponse(requests[k])==-1) deleteRequestStruct(k,requests);
                        break;
                    }
                }
            }
        }
        handleTimeout(requests);
    }
    free(events);
    close(epoolFd);
    close(serverSoc);
}