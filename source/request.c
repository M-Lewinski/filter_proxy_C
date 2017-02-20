#include "request.h"


struct requestStruct * newRequestStruct(){
    struct requestStruct *req = (struct requestStruct*)malloc(sizeof(struct requestStruct));
    req->clientSoc = -1;
    req->serverSoc = -1;
    req->clientRequest = NULL;
    req->serverResponse = NULL;
    req->time = time(0);
    return req;
}

struct request * newRequest(){
    struct request *req = (struct request*)malloc(sizeof(struct request));
    req->headersCount = 0;
    req->cookiesCount = 0;
    req->requestData=NULL;
    req->dataLen=0;
    req->headers=NULL;
    req->cookies=NULL;
    return req;
}

void freeRequest(struct request* req){
    if(req==NULL) return;
    int i=0;
    for(i=0;i<req->headersCount;i++){
        if(req->headers[i].name!=NULL) free(req->headers[i].name);
        if(req->headers[i].value!=NULL) free(req->headers[i].value);
    }
    for(i=0;i<req->cookiesCount;i++){
        if(req->cookies[i].name!=NULL)free(req->cookies[i].name);
        if(req->cookies[i].value!=NULL)free(req->cookies[i].value);
        if(req->cookies[i].cookieAttr!=NULL)free(req->cookies[i].cookieAttr);
    }
    if(req->headers!=NULL) free(req->headers);
    if(req->cookies!=NULL) free(req->cookies);
    if(req->requestData!=NULL) free(req->requestData);
}

void removeRequestStruct(struct requestStruct *req, struct requestStruct ***requests, int *connections, int epoolFd,
                         int *threadCount) {
    (*threadCount)--;
    if(req == NULL){
        return;
    }
    struct epoll_event event;
    event.data.ptr=req;
    event.events = EPOLLIN;
    if(req->clientSoc>0) {
        epoll_ctl(epoolFd,EPOLL_CTL_DEL,req->clientSoc,&event);
        close(req->clientSoc);
    }
    if(req->serverSoc>0){
        epoll_ctl(epoolFd,EPOLL_CTL_DEL,req->serverSoc,&event);
        close(req->serverSoc);
    }
    if(req->clientRequest != NULL) {
        freeRequest(req->clientRequest);
        free(req->clientRequest);
    }
    if(req->serverResponse != NULL) {
        freeRequest(req->serverResponse);
        free(req->serverResponse);
    }
    if((*connections) > 0 && req->clientSoc==(*requests)[(*connections)-1]->clientSoc) (*connections)--;
    else {
        int i;
        for (i = 0; i < *connections; i++) {
            if(req->clientSoc == (*requests)[i]->clientSoc) {
                (*requests)[i]=(*requests)[--(*connections)];
                break;
            }
        }
    }
    free(req);
}


int countRequestLen(struct request req, int type){
    int len = 12, i=0;                  // 10- zapas na 'Cookie: ' nagłówek
    for(i=0; i<req.headersCount;i++)    //miejsce na nagłówki
        len+=(req.headers[i].name!=NULL?strlen(req.headers[i].name):0)+
             (req.headers[i].value!=NULL?strlen(req.headers[i].value):0)+4;
    for(i=0; i<req.cookiesCount;i++)    //miejsce na ciasteczka (Nagłówek 'Cookie' dla type=0 oraz wiele 'Set-Cookie' dla type=1)
        len+=(req.cookies[i].name!=NULL?strlen(req.cookies[i].name):0)+
             (req.cookies[i].value!=NULL?strlen(req.cookies[i].value):0)+
             (req.cookies[i].cookieAttr!=NULL?strlen(req.cookies[i].cookieAttr):0)+6+(type==1?12:0);
    len+=4;
    len+=req.dataLen; //miesce na dane
    return len;
}

char *requestToString(struct request req, int *size, int type){
    int len = countRequestLen(req, type), i=0;
    char *returnString = (char*)malloc(len*sizeof(char));
    returnString[0]='\0';
    for(i=0; i< req.headersCount;i++){
        if(req.headers[i].name!=NULL && strlen(req.headers[i].name)>0){
            strcat(returnString, req.headers[i].name);
            strcat(returnString, ": ");
        }
        if(req.headers[i].value!=NULL)
            strcat(returnString, req.headers[i].value);
        strcat(returnString,"\r\n");
    }
    if(req.cookiesCount>0){
        if(type==0) strcat(returnString,"Cookie: ");
        for(i=0;i<req.cookiesCount;i++){
            if(type==1) strcat(returnString,"Set-Cookie: ");
            strcat(returnString,req.cookies[i].name);
            strcat(returnString,"=");
            strcat(returnString,req.cookies[i].value);
            if(type==1 && req.cookies[i].cookieAttr!=NULL){
                strcat(returnString,"; ");
                strcat(returnString,req.cookies[i].cookieAttr);
            }
            if(type==1 || ((type==0) && (i==req.cookiesCount-1))) strcat(returnString,"\r\n");
            if(type==0 && i!=req.cookiesCount-1) strcat(returnString,"; ");
        }
    }

    strcat(returnString,"\r\n");
    *size= (int) strlen(returnString);
    if(req.dataLen>0) memcpy(&returnString[*size], req.requestData, (size_t) req.dataLen);
    *size+=req.dataLen;
    return returnString;
}


int
readBodyHavingConLen(int content_len, struct request *req, char *ptr, int socket, int bodyreaded, int *threadAlive) {
    int bufSize=4096;
    char buf[bufSize];

    req->requestData = (char *) malloc((content_len + 1) * sizeof(char));
    req->dataLen=content_len;

    if (ptr!=NULL) req->requestData = memcpy(&req->requestData[0], ptr, (size_t) bodyreaded);

    if(socket == -1){
        return 1;
    }
    while(bodyreaded<content_len){
        if(*threadAlive < 0){
            return -1;
        }
        ssize_t read = recv(socket, buf, (size_t) bufSize, 0);
        if(read==-1){
            fprintf(stderr,"Failed to recv form socket when reading body: %d\n",socket);
            return -1;
        }
        if(read==0) return 0;
        memcpy(&req->requestData[bodyreaded], buf, (size_t) read);
        bodyreaded+=read;
    }

    return 1;
}


int getChunkSize(int ptr, int *size, char *body, int bodyLen) {
    int i;
    char hexNum[10];
    memset(&hexNum,0,10);
    for(i=ptr ; i<bodyLen ; i++){
        if(i>1 && (body[i]=='\n') && body[i-1]=='\r'){
            body[i-1]='\0';
            strcpy(hexNum,&body[ptr]);
            body[i-1]='\r';
            break;
        }
    }
    if( strlen(hexNum) > 0) *size = (int)strtol(hexNum, NULL, 16);
    else *size=-1;
    return (int) strlen(hexNum);
}


int readBodyIfChunked(struct request *req, char *ptr, int socket, int bodyReaded, int *threadAlive) {
    int bodyMemmory = (4096 > bodyReaded ? 4096 : bodyReaded ), bodyLen=0;
    req->requestData = (char*)malloc(sizeof(char)*(bodyMemmory+1));
//    req->requestData[0]='\0';

    int size = -1;
    int iptr = -1, numberS = 0;
    if (ptr!=NULL) {
        memcpy(req->requestData, ptr, (size_t) bodyReaded);
        bodyLen+=bodyReaded;
        iptr = 0;
    }
    if(socket == -1) size = 0;
    else numberS = getChunkSize(iptr, &size, req->requestData, bodyLen);

    while(size !=0){
        if(size>0 && iptr+size+numberS+4 < bodyLen) {
            iptr+=size+4+numberS;
            size=-1;
        }
        else {
            int tmpBudSize = 4098;
            char tmpBuf[tmpBudSize+1];
            if(*threadAlive < 0){
                return -1;
            }
            ssize_t read = recv(socket, tmpBuf, (size_t) tmpBudSize, 0);
            if(read==-1){
                fprintf(stderr,"Failed to recv form socket when reading body: %d\n",socket);
                return -1;
            }
            if(read==0) return 0;
            tmpBuf[read]='\0';
            if(bodyLen+read>=bodyMemmory) {
                bodyMemmory  += (int) (2*read);
                req->requestData = (char*) realloc(req->requestData,sizeof(char)*(bodyMemmory+1));
            }
            memcpy(&req->requestData[bodyLen], tmpBuf, (size_t) read);
            bodyLen+=read;
            if(iptr==-1)iptr=0;
        }

        numberS = getChunkSize(iptr, &size, req->requestData,bodyLen);
    }
    req->dataLen=bodyLen;
    return 1;
}

void parseSingleCookie(struct headerCookie *cookie, char* cookieNameVal){
    if(cookieNameVal==NULL || strlen(cookieNameVal)<1) return;
    while(cookieNameVal[0] == ' ') cookieNameVal++;
    int i, len;
    for(i=0;i<strlen(cookieNameVal);i++)
        if(cookieNameVal[i]=='='){
            cookieNameVal[i]='\0';
            break;
        }
    len = (int) strlen(cookieNameVal);
    cookie->cookieAttr=NULL;
    cookie->name = (char*)malloc(sizeof(char)*(len+1));
    strcpy(cookie->name, cookieNameVal);
    cookieNameVal+=len+1;
    len = (int) strlen(cookieNameVal);
    if(cookieNameVal[0]=='"' && cookieNameVal[len-1]=='"') {
        cookieNameVal[len-1] = '\0';
        cookieNameVal++;
        len-=2;
    }
    cookie->value = (char*)malloc(sizeof(char)*(len+1));
    strcpy(cookie->value, cookieNameVal);
}

void parseCookies(struct request *req) {
    int i;
    for (i = 0; i < req->headersCount; i++) {
        if (req->headers[i].name != NULL && !strcmp("Cookie", req->headers[i].name)) {
            int cookiesMem = 5;
            req->cookies = (struct headerCookie *) malloc(cookiesMem * sizeof(struct headerCookie));
            char *tok;
            char *rest = req->headers[i].value;
            while ((tok = strtok_r(rest, ";", &rest))) {
                req->cookiesCount += 1;
                if (req->cookiesCount > cookiesMem) {
                    cookiesMem *= 2;
                    req->cookies = (struct headerCookie *) realloc(req->cookies,
                                                                   cookiesMem * sizeof(struct headerCookie));
                }
                parseSingleCookie(&req->cookies[req->cookiesCount - 1], tok);
            }

            free(req->headers[i].name);
            if (req->headers[i].value != NULL) free(req->headers[i].value);
            if (i != req->headersCount - 1)
                req->headers[i] = req->headers[req->headersCount - 1];
            req->headersCount--;
            break;
        }
    }
}

int readData(struct request *req, int socket, struct requestStruct *requestStruct, int *threadAlive) {
    int j=0,i=0, bufSize = 4096, allRead=0, headersNum=0, loop=1;
    char buf[bufSize+1];
    ssize_t read;
    int requestMem=4096;
    char *request = (char*)malloc(requestMem*sizeof(char));
    request[0]='\0';
    while(loop) {
        if(*threadAlive < 0){
            return -1;
        }
        read=recv(socket, buf, (size_t) bufSize, 0);
        if(read<0){
            fprintf(stderr,"READ ERROR: %s\n",strerror(errno));
            free(request);
            fprintf(stderr,"Failed to recv form socket when reading headers: %d\n",socket);
            return -1;
        }
        if(read == 0) break;

        buf[read]='\0';
        if(allRead+read+1 > requestMem){
            requestMem*=2;
            request = (char*)realloc(request,requestMem*sizeof(char));
        }
        memcpy(&request[allRead], buf, (size_t) read);
        allRead+=read;
        for(j=0;j<read;j++)
            if(buf[j]=='\n'){
                if((allRead>1) && (request[allRead-read+j-2] == '\n')) {
                    loop=0;
                    break;
                } else headersNum++;
            }
    }
    int content_len=0;
    int chunkType=0;
    req->headers = (struct headerCookie*)malloc(headersNum*sizeof(struct headerCookie));
    req->headersCount = headersNum;
    for(i=0;i<headersNum;i++){
        req->headers[i].name=NULL;
        req->headers[i].value=NULL;
        req->headers[i].cookieAttr=NULL;
    }
    char *ptr = request;
    char *delimiter = "\r";
    char *tok = strtok_r(ptr,delimiter,&ptr);

    for(i=0;tok!=NULL && i<headersNum; i++){
        if(i==0) {
            req->headers[i].value = (char *) malloc(sizeof(char) * (strlen(tok)+1));
            strcpy(req->headers[i].value, tok);
            allRead-=(strlen(tok)+2);
        } else{
            for(j=0;j<strlen(tok);j++){
                if(tok[j]==':') {
                    tok[j]='\0';
                    req->headers[i].name = (char*) malloc(sizeof(char) * (strlen(tok)+1));
                    strcpy(req->headers[i].name,tok);
                    allRead-=(strlen(tok)+2);
                    tok = tok+j+2;
                    req->headers[i].value = (char*) malloc(sizeof(char) * (strlen(tok)+1));
                    strcpy(req->headers[i].value,tok);
                    allRead-=(strlen(tok)+2);
                    break;
                }
            }
        }

        if(i<headersNum-1){
            ptr++;
            tok = strtok_r(ptr,delimiter, &ptr);
        }
        if(req->headers[i].name!=NULL && !strcmp(req->headers[i].name,"Content-Length")) content_len = atoi(req->headers[i].value);
        if(req->headers[i].name!=NULL && !strcmp(req->headers[i].name,"Transfer-Encoding") && !strcmp(req->headers[i].value,"chunked")) chunkType=1;
    }
    parseCookies(req);
    if(allRead>3) ptr+=3;
    else ptr=NULL;
    allRead-=2;

    int status = 1;
    if(content_len>0) {
        status = readBodyHavingConLen(content_len, req, ptr, socket, allRead, threadAlive);
    } else {
        if (chunkType){
            status = readBodyIfChunked(req, ptr, socket, allRead, threadAlive);
        } else {
            req->requestData=NULL;
        }
    }

    free(request);
    return status;
}
