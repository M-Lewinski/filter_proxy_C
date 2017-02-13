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
    req->requestData=NULL;
    req->headers=NULL;
    return req;
}

char *requestToString(struct request req, int type){
    //0: request
    //1: repsonse
    //TODO IMPLEMENT THIS
    return NULL;
}
