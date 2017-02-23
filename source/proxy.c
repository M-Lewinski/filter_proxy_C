#include <wchar.h>
#include "proxy.h"


int consoleDesc = 0;
int serverSoc = -1;
struct configStruct* configStructure=NULL;
const int timeout = 5000;



struct threadParametrs *newThread(int epollFd, pthread_mutex_t *mutex, struct requestStruct *req, struct requestStruct ***requests,
                                  int *connections, int *threadCount, int *threadAlive, pthread_t *threadId, pthread_cond_t *endCondition) {
    struct threadParametrs* thread = (struct threadParametrs*)malloc(sizeof(struct threadParametrs));
    thread->connections = connections;
    thread->threadCount = threadCount;
    thread->threadAlive = threadAlive;
    thread->req = req;
    thread->requests = requests;
    thread->epollFd = epollFd;
    thread->requestMutex = mutex;
    thread->threadId = threadId;
    thread->endCondition = endCondition;
    return thread;
}


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
    //struct timeval tv = {5, 0};
    //setsockopt(pStruct->clientSoc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    clientEvent.data.ptr=pStruct;
    clientEvent.events = EPOLLIN | EPOLLONESHOT;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, pStruct->clientSoc, &clientEvent)==-1){
        perror ("epoll_ctl");
        close(pStruct->clientSoc);
        pStruct->clientSoc = -1;
        return -1;
    }
    return 0;
}

int handleRequest(struct requestStruct *reqStruct, int epoolFd, int *threadAlive) {
    reqStruct->clientRequest = newRequest();

    if(readData(reqStruct->clientRequest, reqStruct->clientSoc, threadAlive) < 0){
        return -1;
    }

    if(checkBlocked(configStructure,reqStruct)){
        sendAll(reqStruct->clientSoc, response403, (int) strlen(response403) + 1);
        return -1;
    }
    filterRequest(configStructure,reqStruct);

    if((reqStruct->serverSoc= sendRequest(reqStruct, epoolFd, threadAlive)) < 0){
        return -1;
    }
    return 0;
}

int handleServerResponse(struct requestStruct *reqStruct, int *threadAlive) {
    reqStruct->serverResponse = newRequest();

    if(readData(reqStruct->serverResponse, reqStruct->serverSoc, threadAlive) < 0){
        return -2;
    }
    filterResponse(configStructure, reqStruct);

    int size;
    char* req = requestToString(*reqStruct->serverResponse,&size,1);
    int status = sendAll(reqStruct->clientSoc, req, size);
    free(req);
    if(status<0) return -2;
    return -1;
};

void startProxyServer(char *port, char*address, struct configStruct* config){
    configStructure=config;
    int max_events = 512;
    int* connections = (int*)malloc(sizeof(int));
    *connections = 0;
    int maxConnections = 256;

    struct epoll_event *events = (struct epoll_event *)malloc(max_events * sizeof(struct epoll_event));
    struct requestStruct ***requests = (struct requestStruct***)malloc(sizeof(struct requestStruct**));
    struct requestStruct **requestsArray = (struct requestStruct**)malloc(maxConnections * sizeof(struct requestStruct*));
    requests[0] = requestsArray;
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
    serverInEvent.events = EPOLLIN | EPOLLRDHUP;
    consoleEvent.events = EPOLLIN | EPOLLRDHUP;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, serverSoc, &serverInEvent)==-1){
        perror ("epoll_ctl");
        close(serverSoc);
        close(epoolFd);
        return;
    }
    epoll_ctl(epoolFd, EPOLL_CTL_ADD, 0, &consoleEvent);
    int *threadsCount = (int*)malloc(sizeof(int));
    *threadsCount = 0;
    int *threadAlive = (int*)malloc(sizeof(int));
    *threadAlive = 0;
    pthread_attr_t *attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attr);
    pthread_attr_setdetachstate(attr,PTHREAD_CREATE_DETACHED);
    pthread_mutex_t *mutexRequest = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutexRequest,NULL);
    pthread_cond_t *endCondition = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(endCondition,NULL);

    int pc;
    int loop=1;
    while(loop){
        int n, i;
        n = epoll_wait(epoolFd, events, max_events, 500);
        if(n>(int)(max_events*0.8)){
            max_events*=2;
            events = realloc(events,max_events*sizeof(struct epoll_event));
        }
        for(i=0;i<n;i++){
            if(consoleDesc == *(int*)events[i].data.ptr){ //Handle console input to stop server
                loop=handleConsoleConnection();
            } else if(serverSoc == *(int*)events[i].data.ptr){ //Incoming connection
                pthread_mutex_lock(mutexRequest);
                if(*connections >= (maxConnections-5)){
                    maxConnections*=2;
                    *requests = (struct requestStruct**)realloc(*requests,maxConnections * sizeof(struct requestStruct*));
                }
                pthread_mutex_unlock(mutexRequest);

                struct requestStruct *req = newRequestStruct();
                if (handleNewConnection(serverSoc, epoolFd, req) == 0) {
                    pthread_mutex_lock(mutexRequest);
                    (*requests)[*connections] = req;
                    (*connections)++;
                    pthread_mutex_unlock(mutexRequest);
                } else{
                  free(req);
                }
            } else { //Other events - read data from client and send it to server or from server and send it to client.
                struct requestStruct *reqPtr = (struct requestStruct *)events[i].data.ptr;
                pthread_t *thread = (pthread_t*)malloc(sizeof(pthread_t));
                struct threadParametrs* param = newThread(epoolFd, mutexRequest, reqPtr, requests, connections,
                                                          threadsCount, threadAlive, thread, endCondition);
                pthread_mutex_lock(mutexRequest);
                reqPtr->time = time(0);
                pthread_mutex_unlock(mutexRequest);

                if(reqPtr->serverSoc != -1 && !reqPtr->serverHandled){
                    pthread_mutex_lock(mutexRequest);
                    if(reqPtr->serverHandled) {
                        freethreadParametrs(param);
                        pthread_mutex_unlock(mutexRequest);
                        continue;
                    }
                    (*threadsCount)++;
                    reqPtr->serverHandled=1;
                    pthread_mutex_unlock(mutexRequest);

                    if((pc = pthread_create(thread,attr,threadHandleServerResponse,(void *) param))){
                        pthread_mutex_lock(mutexRequest);
                        (*threadsCount)--;
                        freethreadParametrs(param);
                        fprintf(stderr,"ERROR Pthread_create: %d\n",pc);
                        pthread_mutex_unlock(mutexRequest);
                        continue;
                    }

                } else if (reqPtr->clientSoc != -1 && !reqPtr->clientHandled){
                    pthread_mutex_lock(mutexRequest);
                    if(reqPtr->clientHandled) {
                        freethreadParametrs(param);
                        pthread_mutex_unlock(mutexRequest);
                        continue;
                    }
                    (*threadsCount)++;
                    reqPtr->clientHandled=1;
                    pthread_mutex_unlock(mutexRequest);

                    if((pc = pthread_create(thread,attr,threadHandleClientRequest,(void *) param))){
                        pthread_mutex_lock(mutexRequest);
                        (*threadsCount)--;
                        freethreadParametrs(param);
                        fprintf(stderr,"ERROR Pthread_create: %d\n",pc);
                        pthread_mutex_unlock(mutexRequest);
                        continue;
                    }
                } else{
                    freethreadParametrs(param);
                }
            }
        }
        pthread_mutex_lock(mutexRequest);
        time_t end = time(0);
        for(i=0;i<*connections;i++) {
            if( (*requests)[i]->time - end > timeout ){
                puts("TIMEOUT");

                sendAll((*requests)[i]->clientSoc, proxyTimeout, (int) strlen(proxyTimeout) + 1);
                removeRequestStruct((*requests)[i], requests, connections, epoolFd, NULL);
                i--;
            }
        }
        pthread_mutex_unlock(mutexRequest);
    }
    pthread_mutex_lock(mutexRequest);
    *threadAlive = -1;
    pthread_mutex_unlock(mutexRequest);
    pthread_mutex_lock(mutexRequest);
    if(*threadsCount != 0){
        pthread_cond_wait(endCondition,mutexRequest);
    }
    pthread_mutex_unlock(mutexRequest);
    while(*connections > 0){
         removeRequestStruct((*requests)[0],requests,connections,epoolFd,NULL);
    }
    free(connections);
    free(threadAlive);
    free(threadsCount);
    free(*requests);
    free(requests);
    free(events);
    close(epoolFd);
    close(serverSoc);
    pthread_attr_destroy(attr);
    pthread_mutex_destroy(mutexRequest);
    pthread_cond_destroy(endCondition);
    free(endCondition);
    free(mutexRequest);
    free(attr);
}

int sendRequest(struct requestStruct *request, int epoolFd, int *threadAlive) {
    if(request->serverSoc < 0){
        int newServerSocket = -1;
        struct addrinfo * serverInfo, hints;
        memset(&hints,0,sizeof hints);
        hints.ai_flags=0;
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_STREAM;
        int result;
        char *hostName = getHost(request->clientRequest);
        if(hostName == NULL){
            return -1;
        }
        char *port;
        char *tok = strtok_r(hostName,":",&port);
        if(strcmp(port,"")==0){
            port = "http";
        }
        if ((result = getaddrinfo(tok,port,&hints,&serverInfo))){
            fprintf(stderr,"GETADDRINFO ERROR %s\n",tok);
            if(result == EAI_SYSTEM){
                fprintf(stderr,"GETADDRINFO: %s\n",strerror(errno));
            } else{
                fprintf(stderr,"GETADDRINFO: %s\n",gai_strerror(result));
            }
            return -1;
        }
        if((newServerSocket = socket(AF_INET,SOCK_STREAM,0)) < 0){
            fprintf(stderr,"SERVER SOCKET ERROR\n");
            return -1;
        }
        if(connect(newServerSocket,serverInfo->ai_addr,serverInfo->ai_addrlen) < 0){
            close(newServerSocket);
            return -1;
        }

        freeaddrinfo(serverInfo);
        request->serverSoc = newServerSocket;
        struct timeval tv = {5, 0};
        setsockopt(request->serverSoc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

        struct epoll_event serverEvent;
        serverEvent.data.ptr=request;
        serverEvent.events = EPOLLIN | EPOLLONESHOT;
        if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, request->serverSoc, &serverEvent) < 0){
            perror ("epoll_ctl");
            close(request->serverSoc);
            request->serverSoc = -1;
            return -1;
        }
    }
    int size;
    char* req = requestToString(*request->clientRequest,&size,0);
    if(sendAll(request->serverSoc, req, size) < 0){
        free(req);
        return -1;
    }
    free(req);
    return request->serverSoc;
}

int sendAll(int socket, char *text, int size) {
    if (size == 0){
        fprintf(stderr,"EMPTY BUFFER\n");
        return -1;
    }
    ssize_t sentBytes = send(socket, text, (size_t) size, 0);
    if (sentBytes < 0){
        fprintf(stderr,"ERROR SEND: %s\n",strerror(errno));
        return -1;
    }
    return 1;
}

void *threadHandleServerResponse(void *thread){
    struct threadParametrs* pointer = (struct threadParametrs*) thread;
    struct requestStruct* req = pointer->req;
    int epollFd = pointer->epollFd;
    int *connections = pointer->connections;
    int *threadAlive = pointer->threadAlive;
    int *threadCount = pointer->threadCount;
    pthread_mutex_t* requestMutex = pointer->requestMutex;
    struct requestStruct ***requests = pointer->requests;
    int status = handleServerResponse(req, threadAlive);
    pthread_mutex_lock(requestMutex);
    removeRequestStruct(req, requests, connections, epollFd, threadCount);
    if(*threadAlive < 0 && *threadCount == 0) {
        pthread_cond_signal(pointer->endCondition);
    }
    pthread_mutex_unlock(requestMutex);
    freethreadParametrs(pointer);
}

void *threadHandleClientRequest(void *thread){
    struct threadParametrs* pointer = (struct threadParametrs*) thread;
    struct requestStruct* req = pointer->req;
    int epollFd = pointer->epollFd;
    int *connections = pointer->connections;
    int *threadAlive = pointer->threadAlive;
    int *threadCount = pointer->threadCount;
    struct requestStruct ***requests = pointer->requests;
    pthread_mutex_t *requestMutex = pointer->requestMutex;
    int status = handleRequest(req, epollFd, threadAlive);
    pthread_mutex_lock(requestMutex);
    if(status == -1 || (*threadAlive < 0))
        removeRequestStruct(req, requests, connections, epollFd, threadCount);
    else{
        (*threadCount)--;
    }
    if(*threadAlive < 0 && *threadCount == 0){
        pthread_cond_signal(pointer->endCondition);
    }
    pthread_mutex_unlock(requestMutex);
    freethreadParametrs(pointer);
}


void freethreadParametrs(struct threadParametrs* param){
    if(param->threadId!=NULL){
        free(param->threadId);
        param->threadId = NULL;
    }
    free(param);
}