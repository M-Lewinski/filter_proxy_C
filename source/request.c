#include "request.h"

int maxTimeMsc = 5000;

struct requestStruct newRequestStruct(){
    struct requestStruct req;
    req.clientSoc = -1;
    req.serverSoc = -1;
    req.clientRequest = NULL;
    req.serverResponse = NULL;
    req.time = time(0);
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

void removeRequestStruct(struct requestStruct req, struct requestStruct *requests, int *connections){
    if(req.clientSoc!=-1) close(req.clientSoc);
    if(req.serverSoc!=-1) close(req.serverSoc);
    if(req.clientRequest != NULL) freeRequest(req.clientRequest);
    if(req.serverResponse != NULL) freeRequest(req.serverResponse);
    if(req.clientSoc==requests[(*connections)-1].clientSoc) (*connections)--;
    else {
        int i;
        for (i = 0; i < *connections; i++) {
            if(req.clientSoc == requests[i].clientSoc) {
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
             (req.headers[i].value!=NULL?strlen(req.headers[i].value):0)+3;
    for(i=0; i<req.cookiesCount;i++)    //miejsce na ciasteczka (Nagłówek 'Cookie' dla type=0 oraz wiele 'Set-Cookie' dla type=1)
        len+=(req.cookies[i].name!=NULL?strlen(req.cookies[i].name):0)+
             (req.cookies[i].value!=NULL?strlen(req.cookies[i].value):0)+
             (((req.cookies[i].cookieAttr!=NULL) && (type==1))?strlen(req.cookies[i].cookieAttr):0)+4+
              type==1?12:0;
    len+=2;
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
void readData(struct request *req, int socket, time_t timeR) {
    int j=0,i=0,k=0, bufSize = 100, allRead=0, headersNum=0, loop=1;
    char buf[bufSize+1];
    ssize_t read;
    int requestMem=200;
    char *request = (char*)malloc(requestMem*sizeof(char));
    request[0]='\0';
    while( loop && ((read=recv(socket, buf, (size_t) bufSize, 0)) > 0)) {
        time_t waitTime = maxTimeMsc - (time(0) - timeR);
        if(waitTime<0) {
            free(request);
            return;
        }

        buf[read]='\0';
        printf("%s\n",buf);
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
