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
    req->headers=NULL;
    req->cookies=NULL;
    return req;
}

void freeRequest(struct request* req){
    if(req==NULL) return;
    int i=0;
    for(i=0;i<req->headersCount;i++){
        if(req->headers[i].name!=NULL)free(req->headers[i].name);
        if(req->headers[i].value!=NULL)free(req->headers[i].value);
    }
    for(i=0;i<req->cookiesCount;i++){
        if(req->cookies[i].name!=NULL)free(req->cookies[i].name);
        if(req->cookies[i].value!=NULL)free(req->cookies[i].value);
        if(req->cookies[i].cookieAttr!=NULL)free(req->cookies[i].cookieAttr);
    }
    free(req->headers);
    free(req->cookies);
    free(req->requestData);
}

void removeRequestStruct(struct requestStruct req, struct requestStruct **requests, int *connections){
    if(req.clientSoc!=-1) close(req.clientSoc);
    if(req.serverSoc!=-1) close(req.serverSoc);
    if(req.clientRequest != NULL) freeRequest(req.clientRequest);
    if(req.serverResponse != NULL) freeRequest(req.serverResponse);
    if(req.clientSoc==requests[(*connections)-1]->clientSoc) (*connections)--;
    else {
        int i;
        for (i = 0; i < *connections; i++) {
            if(req.clientSoc == requests[i]->clientSoc) {
                requests[i]=requests[--(*connections)];
                break;
            }
        }
    }
}

int countRequestLen(struct request req, int type){
    int len = 10, i=0;                  // 10- zapas na 'Cookie: ' nagłówek
    for(i=0; i<req.headersCount;i++)    //miejsce na nagłówki
        len+=(req.headers[i].name!=NULL?strlen(req.headers[i].name):0)+
             (req.headers[i].value!=NULL?strlen(req.headers[i].value):0)+4;
    for(i=0; i<req.cookiesCount;i++)    //miejsce na ciasteczka (Nagłówek 'Cookie' dla type=0 oraz wiele 'Set-Cookie' dla type=1)
        len+=(req.cookies[i].name!=NULL?strlen(req.cookies[i].name):0)+
             (req.cookies[i].value!=NULL?strlen(req.cookies[i].value):0)+
             (((req.cookies[i].cookieAttr!=NULL) && (type==1))?strlen(req.cookies[i].cookieAttr):0)+5+
              type==1?12:0;
    len+=4;
    len+=req.requestData!=NULL?strlen(req.requestData):0; //miesce na dane
    return len;
}

char *requestToString(struct request req, int type){
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
    if(req.requestData!=NULL) strcat(returnString,req.requestData);
    return returnString;
}


/*
 * METHOD TO FULL CHANGE
 */
int readData(struct request *req, int socket, time_t timeR) {
    int j=0,i=0, bufSize = 4096, allRead=0, headersNum=0, loop=1;
    char buf[bufSize+1];
    ssize_t read;
    int requestMem=4096;
    char *request = (char*)malloc(requestMem*sizeof(char));
    request[0]='\0';

    while(loop) {
        read=recv(socket, buf, (size_t) bufSize, 0);
        if(read<0){
            fprintf(stderr,"READ ERROR: %s\n",strerror(errno));
            free(request);
            //fprintf(stderr,"Failed to recv form socket when reading headers: %d\n",socket);
            return -1;
        }

        buf[read]='\0';
        if(allRead+read+1 > requestMem){
            requestMem*=2;
            request = (char*)realloc(request,requestMem*sizeof(char));
        }
        strcat(request,buf);
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
    req->headers = (struct headerCookie*)malloc(headersNum*sizeof(struct headerCookie));
    req->headersCount = headersNum;
    char *ptr = request;
    char *tok = strtok_r(request,"\r\n",&ptr);
    for(i=0;tok!=NULL && i<headersNum; i++){
        req->headers[i].name=NULL;
        req->headers[i].value=NULL;
        req->headers[i].cookieAttr=NULL;
        if(i==0) {
            req->headers[i].value = (char *) malloc(sizeof(char) * strlen(tok));
            req->headers[i].value = strcpy(req->headers[i].value, tok);
        } else{
            for(j=0;j<strlen(tok);j++){
                if(tok[j]==':') {
                    tok[j]='\0';
                    req->headers[i].name = (char*) malloc(sizeof(char) * strlen(tok));
                    req->headers[i].name = strcpy(req->headers[i].name,tok);
                    tok = tok+j+2;
                    req->headers[i].value = (char*) malloc(sizeof(char) * strlen(tok));
                    req->headers[i].value = strcpy(req->headers[i].value,tok);
                    break;
                }
            }
        }

        if(i<headersNum-1)tok = strtok_r(ptr,"\r\n", &ptr);
        if(req->headers[i].name!=NULL && !strcmp(req->headers[i].name,"Content-Length")) content_len = atoi(req->headers[i].value);
    }

    if(content_len>0) {
        req->requestData = (char *) malloc((content_len + 1) * sizeof(char));
        req->requestData[0]='\0';
        if (ptr!=NULL && strlen(ptr)>3) req->requestData = strcpy(req->requestData, ptr+3);
        content_len-=strlen(ptr);
        while(content_len>0){
            read = recv(socket, buf, (size_t) bufSize, 0);
            if(read==-1){
                free(request);
                fprintf(stderr,"Failed to recv form socket when reading body: %d\n",socket);
                return -1;
            }
            buf[read]='\0';
            strcat(req->requestData, buf);
            content_len-=read;
        }
    } else req->requestData=NULL;

    free(request);
    return 0;
}
