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

void removeRequestStruct(int requestIndex, struct requestStruct **requests, int *connections){
    struct requestStruct *req = requests[requestIndex];
    if(req->clientSoc!=-1) close(req->clientSoc);
    if(req->serverSoc!=-1) close(req->serverSoc);
    if(req->clientRequest != NULL) freeRequest(req->clientRequest);
    if(req->serverResponse != NULL) freeRequest(req->serverResponse);
    if(requestIndex==(*connections)-1) (*connections)--;
    else requests[requestIndex] = requests[--(*connections)];
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
