#include "config.h"

struct configStruct* loadConfig(){
    return loadConfigWithPath("../conf.ini");
};
int blockRuleMalloc=5;
int headerRuleMalloc=5;
int cookieRuleMalloc=5;
struct configStruct* loadConfigWithPath(char* filePath){
    FILE *file = fopen(filePath, "r");
    if(file==NULL){
        fprintf(stderr,"Failed to open file");
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t lineLen = 0;
    struct configStruct *config = NULL;
    config = (struct configStruct*)malloc(1* sizeof(struct configStruct));
    config->blockRulesNumber = 0;
    config->cookieRulesNumber = 0;
    config->headerRulesNumber = 0;
    config->block = (struct blockRule*)malloc(blockRuleMalloc*sizeof(struct blockRule));
    config->header = (struct headersRule*)malloc(headerRuleMalloc* sizeof(struct headersRule));
    config->cookie = (struct headersRule*)malloc(cookieRuleMalloc* sizeof(struct headersRule));

    while ((lineLen = getline(&line, &len, file)) != -1)
        if(parseLine(config, line, lineLen)!=0)
            fprintf(stderr, "Failed to parse single line: %s, skipping it\n",line);

    if(line!=NULL) free(line);
    fclose(file);
    return config;
}

struct headersRule newClearRule(){
    struct headersRule r;
    r.hostNamePattern=NULL;
    r.hostNameRegex=NULL;
    r.value=NULL;
    r.namePattern=NULL;
    r.nameRegex=NULL;
    return r;
}
void freeHeaderStruct(struct headersRule ruleToFree){
    if(ruleToFree.hostNameRegex!=NULL) regfree(ruleToFree.hostNameRegex);
    if(ruleToFree.nameRegex!=NULL) regfree(ruleToFree.nameRegex);
    free(ruleToFree.namePattern);
    free(ruleToFree.hostNamePattern);
    free(ruleToFree.value);
}
void freeBlockStruct(struct blockRule ruleToFree){
    free(ruleToFree.hostPattern);
    if(ruleToFree.hostRegex!=NULL) regfree(ruleToFree.hostRegex);
}

void freeConfig(struct configStruct* config){
    if(config==NULL) return;

    int i;
    for(i=0;i<config->blockRulesNumber;i++)
        freeBlockStruct(config->block[i]);
    for(i=0;i<config->cookieRulesNumber;i++)
        freeHeaderStruct(config->cookie[i]);
    for(i=0;i<config->headerRulesNumber;i++)
        freeHeaderStruct(config->header[i]);
    free(config->cookie);
    free(config->block);
    free(config->header);
    free(config);
};

int parseLine(struct configStruct* config, char* line, ssize_t lineLen){
    int i;
    if(line[lineLen-1]=='\n') line[lineLen-1]='\0'; //Removing new line
    while(line[0]=='\t' || line[0]==' ') line=line+1; //Removing whitespaces
    if(line[0]=='#' || line[0]=='\0') return 0; //Skipping empty lines and comments

    int maxPartCount=10, partCount=0;
    char** splittedLine = (char**)malloc(maxPartCount*sizeof(char*));
    for(i=0;i<maxPartCount;i++) splittedLine[i]="";
    char*lineCpy=(char*)malloc((strlen(line)+1)*sizeof(char));
    lineCpy = strcpy(lineCpy,line);

    char *tok = strtok(lineCpy," ");
    while(tok!=NULL){
        splittedLine[partCount]=tok;
        partCount++;
        tok = strtok(NULL," ");
        if(maxPartCount<=partCount){
            maxPartCount+=2;
            splittedLine=realloc(splittedLine,maxPartCount*sizeof(char*));
        }
    }

    int toReturn;
    if(!strcmp(splittedLine[0],"BLOCK"))
        toReturn=parseBlockRule(config, splittedLine);
    else if (!strcmp(splittedLine[0],"HEADER"))
        toReturn=parseHeaderRule(config, splittedLine,0);
    else if (!strcmp(splittedLine[0],"COOKIE"))
        toReturn=parseHeaderRule(config, splittedLine,1);
    else toReturn=-1;

    free(splittedLine);
    free(lineCpy);
    return toReturn;
};

int parseBlockRule(struct configStruct* config, char** splittedLine){
    char* blockPattern = splittedLine[1];
    if(strlen(blockPattern)<1)return -1;
    struct blockRule blockR;

    blockR.hostPattern=(char*)malloc((strlen(blockPattern)+1)*sizeof(char));
    blockR.hostRegex = (regex_t*)malloc(sizeof(regex_t));
    strcpy(blockR.hostPattern,blockPattern);

    if(regcomp(blockR.hostRegex,blockR.hostPattern,0)){
        fprintf(stderr,"Unable to compile regex: %s",blockR.hostPattern);
        freeBlockStruct(blockR);
        return -1;
    }
    if(config->blockRulesNumber>=blockRuleMalloc){
        blockRuleMalloc++;
        config->block=realloc(config->block,blockRuleMalloc*sizeof(struct blockRule));
    }
    config->block[config->blockRulesNumber]=blockR;
    config->blockRulesNumber++;
    return 0;
}

int parseHeaderRule(struct configStruct* config, char** splittedLine, int headerCookie){
    enum ruleType type;
    if(!strcmp(splittedLine[1],"DEL")) type = DEL;
    else if(!strcmp(splittedLine[1],"ADD")) type = ADD;
    else if(!strcmp(splittedLine[1],"CHA")) type = CHA;
    else return -1;

    struct headersRule rule = newClearRule();
    rule.type=type;

    char* name=splittedLine[2];     //Compiling header name regex
    if(strlen(name)<1) return -1;
    rule.namePattern = (char*)malloc((strlen(name)+1)*sizeof(char));
    strcpy(rule.namePattern,name);
    rule.nameRegex=(regex_t*)malloc(sizeof(regex_t));
    if(regcomp(rule.nameRegex,rule.namePattern,0)){
        fprintf(stderr,"Unable to compile regex: %s\n",name);
        freeHeaderStruct(rule);
        return -1;
    }

    if(type!=DEL){                  //Value if type is not DELETE
        char *value=splittedLine[3];
        if(strlen(value)<1) {
            freeHeaderStruct(rule);
            return -1;
        }
        rule.value = (char*)malloc((strlen(value)+1)*sizeof(char));
        strcpy(rule.value,value);
    }

    char* host;                     //Compiling host regex
    if(type!=DEL) host=splittedLine[4];
    else host=splittedLine[3];
    if(strlen(host)>1) {
        rule.hostNamePattern = (char*)malloc((strlen(host)+1)*sizeof(char));
        strcpy(rule.hostNamePattern,host);
        rule.hostNameRegex=(regex_t*)malloc(sizeof(regex_t));
        if (regcomp(rule.hostNameRegex, rule.hostNamePattern, 0)) {
            fprintf(stderr, "Unable to compile regex: %s\n", host);
            freeHeaderStruct(rule);
            return -1;
        }
    }

    switch (headerCookie){
        case 0:
            if(config->headerRulesNumber>=headerRuleMalloc){
                headerRuleMalloc++;
                config->header=realloc(config->header,headerRuleMalloc*sizeof(struct blockRule));
            }
            config->header[config->headerRulesNumber]=rule;
            config->headerRulesNumber++;
            break;
        case 1:
            if(config->cookieRulesNumber>=cookieRuleMalloc){
                cookieRuleMalloc++;
                config->cookie=realloc(config->cookie,cookieRuleMalloc*sizeof(struct blockRule));
            }
            config->cookie[config->cookieRulesNumber]=rule;
            config->cookieRulesNumber++;
            break;
        default:
            exit(1);
    }
    return 0;
}
