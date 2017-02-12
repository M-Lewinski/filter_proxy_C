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

int checkBlocked(struct configStruct *config, struct requestStruct *reqStruct){
    char* location=NULL;
    int i;
    for(i=0;i<reqStruct->clientRequest->headersCount;i++) {
        if (!strcmp(reqStruct->clientRequest->headers[i].name, hostHeader)) {
            location = reqStruct->clientRequest->headers[i].value;
            break;
        }
    }
    if(location==NULL) return 0;
    for(i=0;i<config->blockRulesNumber;i++){
        if(checkRegex(location,config->block[i].hostRegex)) return 1;
    }
    return 0;
}


