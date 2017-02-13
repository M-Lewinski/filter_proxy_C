#include "rule.h"

const char* hostHeader = "Host";

int checkRegex(char* toCheck, regex_t *regex){
    int reti = regexec(regex, toCheck, 0, NULL, 0);
    if (!reti) return 1;
    else if (reti == REG_NOMATCH) return 0;
    else {
        char msgbuf[100];
        regerror(reti, regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        return 0;
    }
}

char *getHost(struct request *req){
    int i=0;
    char *location=NULL;
    for(i=0;i<req->headersCount;i++) {
        if (!strcmp(req->headers[i].name, hostHeader)) {
            location = req->headers[i].value;
            break;
        }
    }
    return location;
}

int checkBlocked(struct configStruct *config, struct requestStruct *reqStruct){
    char* location=NULL;
    int i;
    location=getHost(reqStruct->clientRequest);
    if(location==NULL) return 0;
    for(i=0;i<config->blockRulesNumber;i++){
        if(checkRegex(location,config->block[i].hostRegex)) return 1;
    }
    return 0;
}

struct headerCookie * add(char *name, char *value, struct headerCookie *headers, int *count) {
    int i=0;
    for(i=0;i<*count;i++) if(!strcmp(name,headers[i].name)) return headers; //Check if header not exist
    struct headerCookie newHeader;
    newHeader.cookieAttr=NULL;
    newHeader.name = (char*)malloc((strlen(name)+1)*sizeof(char));
    newHeader.name = strcpy(newHeader.name,name);
    newHeader.value=(char*)malloc((strlen(value)+1)*sizeof(char));
    newHeader.value=strcpy(newHeader.value,value);
    (*count)++;
    headers = (struct headerCookie*)realloc(headers,(*count)*sizeof(struct headerCookie));
    headers[(*count)-1]=newHeader;
    return headers;
}

struct headerCookie *del(regex_t *name, struct headerCookie *headers, int *count) {
    int i=0;
    for(i=0;i<*count;i++)
        if(checkRegex(headers[i].name,name)){
            free(headers[i].value);
            free(headers[i].name);
            free(headers[i].cookieAttr);
            if(i!=*count-1) headers[i] = headers[*count-1];
            (*count)--;
        }
    return (struct headerCookie*)realloc(headers,(*count)*sizeof(struct headerCookie));
}

void cha(regex_t *name, char *value, struct headerCookie *headers, int *count) {
    int i=0;
    for(i=0;i<*count;i++)
        if(checkRegex(headers[i].name,name)){
            if(strlen(headers[i].value) < strlen(value))
                headers[i].value = (char*)realloc(headers[i].value, (strlen(value)+1)*sizeof(char));
            headers[i].value = strcpy(headers[i].value,value);
        }
}

void filterCookieHeader(struct headersRule rule, struct request *req, int headerOrCookie){
    switch (rule.type){
        case DEL:
            if(!headerOrCookie) req->headers = del(rule.nameRegex, req->headers, &req->headersCount);
            else req->cookies = del(rule.nameRegex, req->cookies, &req->cookiesCount);
            break;
        case CHA:
            if(!headerOrCookie) cha(rule.nameRegex, rule.value, req->headers, &req->headersCount);
            else cha(rule.nameRegex, rule.value, req->cookies, &req->cookiesCount);
            break;
        case ADD:
            if(!headerOrCookie) req->headers = add(rule.namePattern, rule.value, req->headers, &req->headersCount);
            else req->cookies = add(rule.namePattern, rule.value, req->cookies, &req->cookiesCount);
            break;
    }
}

void filter(struct configStruct *config, struct request *req, char* host){
    int i=0;
    for(i=0;i<config->headerRulesNumber;i++){
        if((config->header[i].hostNamePattern!=NULL) && (host!=NULL))
            if(!checkRegex(host,config->header[i].hostNameRegex)) break;
        filterCookieHeader(config->header[i],req, 0);
    }
    for(i=0;i<config->cookieRulesNumber;i++){
        if((config->cookie[i].hostNamePattern!=NULL) && (host!=NULL))
            if(!checkRegex(host,config->cookie[i].hostNameRegex)) break;
        filterCookieHeader(config->cookie[i],req, 1);
    }

}

void filterRequest(struct configStruct *config, struct requestStruct *reqStruct){
    char *location = getHost(reqStruct->clientRequest);
    filter(config,reqStruct->clientRequest,location);
}

void filterResponse(struct configStruct *config, struct requestStruct *reqStruct){
    char *location = getHost(reqStruct->clientRequest);
    filter(config,reqStruct->serverResponse,location);
}


