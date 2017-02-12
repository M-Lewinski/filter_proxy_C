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

void addHeader(char* name, char* value, struct request *req){
    int i=0;
    for(i=0;i<req->headersCount;i++) if(!strcmp(name,req->headers[i].name)) return; //Check if header not exist
    struct header newHeader;
    strcpy(newHeader.name,name);
    newHeader.value=(char*)malloc((strlen(name)+1)*sizeof(char));
    newHeader.value=strcpy(newHeader.value,value);
    req->headersCount++;
    req->headers = (struct header*)realloc(req->headers,req->headersCount*sizeof(struct header));
    req->headers[req->headersCount-1]=newHeader;
}

void delHeader(regex_t *name, char* value, struct request *req){
    int i=0;
    for(i=0;i<req->headersCount;i++)
        if(checkRegex(req->headers[i].name,name)){
            free(req->headers[i].value);
            if(i!=req->headersCount-1) req->headers[i] = req->headers[req->headersCount-1];
            req->headersCount--;
        }
    req->headers = (struct header*)realloc(req->headers,req->headersCount*sizeof(struct header));
}

void chaHeader(regex_t *name, char* value, struct request *req){
    int i=0;
    for(i=0;i<req->headersCount;i++)
        if(checkRegex(req->headers[i].name,name)){
            if(strlen(req->headers[i].value) < strlen(value))
                req->headers[i].value = (char*)realloc(req->headers[i].value, (strlen(value)+1)*sizeof(char));
            req->headers[i].value = strcpy(req->headers[i].value,value);
        }
}

void filterHeader(struct headersRule rule, struct request *req){
    switch (rule.type){
        case DEL:
            delHeader(rule.nameRegex,rule.value,req);
            break;
        case CHA:
            chaHeader(rule.nameRegex,rule.value,req);
            break;
        case ADD:
            addHeader(rule.namePattern,rule.value,req);
            break;
    }
}
void filterCookie(struct headersRule rule, struct request *req, char* cookieHeader){
    //TODO IMPLEMENT THIS
}

void filter(struct configStruct *config, struct request *req, char* cookieHeader, char* host){
    int i=0;
    for(i=0;i<config->headerRulesNumber;i++){
        if((config->header[i].hostNamePattern!=NULL) && (host!=NULL))
            if(!checkRegex(host,config->header[i].hostNameRegex)) break;

        filterHeader(config->header[i],req);
        filterCookie(config->header[i],req,cookieHeader);
    }
}

void filterRequest(struct configStruct *config, struct requestStruct *reqStruct){
    char *location = getHost(reqStruct->clientRequest);
    filter(config,reqStruct->clientRequest,"Cookie",location);
}

void filterResponse(struct configStruct *config, struct requestStruct *reqStruct){
    char *location = getHost(reqStruct->clientRequest);
    filter(config,reqStruct->serverResponse,"Set-Cookie",location);
}


