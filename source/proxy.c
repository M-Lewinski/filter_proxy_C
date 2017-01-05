#include "proxy.h"

char *localhost = (char*)"127.0.0.1";
int max_events = 256;

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
    puts("DUPA");
    ssize_t readed = read(0, buf, 4);
    if((readed==4) && (!strcmp("exit",buf))) return 0;
    return 1;
}

void handleNewConnection(int serverFd, int epoolFd) {
    struct epoll_event clientEvent;
    int clientLen;
    struct sockaddr_in clientAddr;
    int clientFd = accept(serverFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    if(clientFd==-1){
        perror("accept");
        fprintf(stderr, "Failed to accept connection");
        return;
    }
    clientEvent.data.fd=clientFd;
    clientEvent.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoolFd, EPOLL_CTL_ADD, clientFd, &clientEvent)==-1);
}

void handleRequest(int clientFd) {
    char buf[1024];
    int j=0;

    for (j=0;j<1024;j++)buf[j]='\0';
    ssize_t readed;
    if( (readed=recv(clientFd, buf, 1024, 0)) > 0) {
        //TODO read headers
        //TODO read form data
        //TODO how to know if end of reading?
        //TODO between headers and data we have empty line which means two \n \n characters
        //TODO in headers we have header with content length. It should be enough but not as easy as I thought.
        //write(1, buf, (size_t) readed);
        for (j=0;j<10;j++)buf[j]='\0';
    }
    //ONLY FOR TEST
    //TODO REMOVE IT AND CREATE FUNCTION TO RETURN RESPONSE
    send(clientFd,response403,strlen(response403),0);
    close(clientFd); //Close connection
}

void startProxyServer(char *port,struct configStruct* config){
    int serverSoc = createAndListenServerSocket(port);
    if(serverSoc==-1) return;

    int epoolFd = epoll_create1(0);
    if (epoolFd == -1) {
        perror ("epoll_create");
        return;
    }

    struct epoll_event *events = (struct epoll_event *)malloc(max_events * sizeof(struct epoll_event));
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
        int n, i;
        n = epoll_wait(epoolFd, events, 100, 5000);
        for(i=0;i<n;i++){
            if(0 == events[i].data.fd){ //Handle console input to stop server
                loop=handleConsoleConnection();
            } else if(serverSoc == events[i].data.fd){ //Incoming connection
                handleNewConnection(serverSoc, epoolFd);
            } else { //Other events - read data and do sth.
                handleRequest(events[i].data.fd);
            }
        }
    }
    free(events);
    close(epoolFd);
    close(serverSoc);
};