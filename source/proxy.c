#include "proxy.h"

int max_events = 256;
int connections = 0;
int maxConnections = 256;
struct configStruct* configStructure=NULL;

int createAndListenServerSocket(char *port, char *address) {
    struct sockaddr_in serverSocAddr;
    if(atoi(port) <=0 || atoi(port) > 65535){
        fprintf(stderr, "Invalid port number");
        return -1;
    }
    serverSocAddr.sin_port = htons((uint16_t) atoi(port));
    serverSocAddr.sin_family = AF_INET;
    if(address==NULL)inet_aton((const char *) INADDR_LOOPBACK, &serverSocAddr.sin_addr);
    else if(inet_aton(address, &serverSocAddr.sin_addr)==0){
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

/*
 * METHOD TO FULL CHANGE
 */
void readData(struct request *req, int socket) {
    int j=0,i=0,k=0, bufSize = 100, allRead=0, headersNum=0, loop=1;
    char buf[bufSize+1];
    ssize_t read;
    int requestMem=200;
    char *request = (char*)malloc(requestMem*sizeof(char));
    request[0]='\0';

    while( loop && ((read=recv(socket, buf, (size_t) bufSize, 0)) > 0)) {
        buf[read]='\0';
        if(allRead+read+1 > requestMem){
            requestMem*=2;
            request = (char*)realloc(request,requestMem*sizeof(char));
        }
        request = strcat(request,buf);
        for(j=0;j<read;j++)
            if(buf[j]=='\n'){
                if((allRead>1) && (request[allRead+j-2] == '\n')) {
                    request[allRead+j-1] = '\0';
                    loop=0;
                    break;
                } else headersNum++;
            }
        allRead+=read;
    }
    int content_len=0;
    char *toFree = request;
    req->headers = (struct headerCookie*)malloc(headersNum*sizeof(struct headerCookie));
    req->headersCount = headersNum;
    for(i=0;i<headersNum;i++){
        req->headers[i].name=NULL;
        req->headers[i].value=NULL;
        req->headers[i].cookieAttr=NULL;
        for(j=0;;j++){
            allRead--;
            if(request[j]=='\0') break;
            if (request[j]=='\n'){
                request[j] = '\0';
                for(k=0;k<j-1;k++) {
                    if ((request[k] == ':') && (request[k + 1] == ' ')) {
                        request[k] = '\0';
                        req->headers[i].value = (char *) malloc((strlen(request + k + 2) + 1) * sizeof(char));
                        req->headers[i].value = strcpy(req->headers[i].value, request + k + 2);
                        break;
                    }
                }
                req->headers[i].name = (char*)malloc((strlen(request))*sizeof(char));
                req->headers[i].name = strcpy(req->headers[i].name,request);
                request = request+j+1;
                break;
            }
        }
        if(!strcmp(req->headers[i].name,"Content-Length")) content_len = atoi(req->headers[i].value);
    }
    allRead-=2;
    if(content_len>0) {
        req->requestData = (char *) malloc((content_len + 1) * sizeof(char));
        req->requestData[0]='\0';
        if (allRead > 0) req->requestData = strcat(req->requestData, request+1);
        content_len-=allRead;
        while(content_len>0){
            read = recv(socket, buf, (size_t) bufSize, 0);
            buf[read]='\0';
            req->requestData = strcat(req->requestData, buf);
            content_len-=read;
        }
    } else req->requestData=NULL;

    free(toFree);
}

int handleRequest(struct requestStruct *reqStruct) {
    reqStruct->clientRequest = newRequest();
    readData(reqStruct->clientRequest, reqStruct->clientSoc);

    if(checkBlocked(configStructure,reqStruct)){
        send(reqStruct->clientSoc,response403,strlen(response403),0);
        return -1;
    }
//    filterRequest(configStructure,reqStruct);
    if(sendRequest(reqStruct)<0){
        return -1;
    }
    //ONLY FOR TEST
    //TODO REMOVE IT AND CREATE FUNCTION TO MAKE CALL TO SERVER
    send(reqStruct->clientSoc,notImplemented,strlen(notImplemented),0);
    return -1;
}

int handleServerResponse(struct requestStruct *reqStruct) {
    //TODO READ DATA FROM SERVER

    filterResponse(configStructure, reqStruct);

    //TODO WRITE RESPONSE TO CLIENT
    //Hmnn, it should always return -1
    return -1;
};

void handleTimeout(struct requestStruct **requests) {
    //TODO REMOVE ALL OLD REQUESTS WITH TIMEOUT AND SEND MESSAGE WITH 504 status to client
}

void startProxyServer(char *port, char*address, struct configStruct* config){
    configStructure=config;

    struct epoll_event *events = (struct epoll_event *)malloc(max_events * sizeof(struct epoll_event));
    struct requestStruct **requests = (struct requestStruct**)malloc(maxConnections * sizeof(struct requestStruct*));

    int serverSoc = createAndListenServerSocket(port, address);
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
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, serverSoc, &serverInEvent)==-1){
        perror ("epoll_ctl");
        return;
    }
    epoll_ctl(epoolFd, EPOLL_CTL_ADD, 0, &consoleEvent);

    int loop=1;
    while(loop){
        int n, i, k;
        n = epoll_wait(epoolFd, events, max_events, 5000);
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
                        if(handleRequest(requests[k])==-1) removeRequestStruct(k, requests, &connections);
                        break;
                    }
                    if (requests[k]->serverSoc != -1 && requests[k]->serverSoc == events[i].data.fd) {
                        if(handleServerResponse(requests[k])==-1) removeRequestStruct(k, requests, &connections);
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