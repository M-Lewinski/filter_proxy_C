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
    config->header = (struct headersRule*)malloc(cookieRuleMalloc* sizeof(struct headersRule));

    while ((lineLen = getline(&line, &len, file)) != -1)
        if(parseLine(config, line, lineLen)!=0)
            fprintf(stderr, "Failed to parse single line: %s, skipping it\n",line);

    if(line!=NULL) free(line);
    fclose(file);
    return config;
}
void freeHeaderStruct(struct headersRule ruleToFree){
    regfree(ruleToFree.hostNameRegex);
    regfree(ruleToFree.nameRegex);
    free(ruleToFree.namePattern);
    free(ruleToFree.hostNamePattern);
    free(ruleToFree.value);
}

void freeConfig(struct configStruct* config){
    int i;
    for(i=0;i<config->blockRulesNumber;i++){
        free(config->block[i].hostPattern);
        regfree(config->block[i].hostRegex);
    }
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

    int maxPartCount=10, partCount=1;
    char** splittedLine = (char**)malloc(maxPartCount*sizeof(char*));
    for(i=0;i<maxPartCount;i++) splittedLine[i]="\0";
    char*lineCpy=(char*)malloc((strlen(line)+1)*sizeof(char));
    strcpy(lineCpy, line);
    splittedLine[0]=lineCpy;
    char* splitPointer;
    for(splitPointer = lineCpy+1;splitPointer[0]!='\0';splitPointer++) { //LINE SPLITTING
        if(maxPartCount<partCount){
            maxPartCount+=1;
            splittedLine=realloc(splittedLine,maxPartCount*sizeof(char*));
        }
        if ((splitPointer[0] != ' ' || splitPointer[0] != '\t') &&
            ((splitPointer - 1)[0] == ' ' || (splitPointer - 1)[0] == '\t')) {
            splittedLine[partCount] = splitPointer;
            (splitPointer - 1)[0] = '\0';
            partCount++;
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
    struct blockRule* blockR = (struct blockRule*)malloc(1*sizeof(struct blockRule));
    blockR->hostPattern=(char*)malloc((strlen(blockPattern)+1)*sizeof(char));
    strcpy(blockR->hostPattern,blockPattern);

    regex_t *reg = (regex_t*)malloc(sizeof(regex_t));
    if(regcomp(reg,blockR->hostPattern,0)){
        fprintf(stderr,"Unable to compile regex: %s",blockR->hostPattern);
        free(blockR);
        return -1;
    }
    blockR->hostRegex=reg;
    if(config->blockRulesNumber>=blockRuleMalloc){
        blockRuleMalloc++;
        config->block=realloc(config->block,blockRuleMalloc*sizeof(struct blockRule));
    }
    config->block[config->blockRulesNumber]=*blockR;
    config->blockRulesNumber++;
    return 0;
}

int parseHeaderRule(struct configStruct* config, char** splittedLine, int headerCookie){
    enum ruleType type;
    if(!strcmp(splittedLine[1],"DEL")) type = DEL;
    else if(!strcmp(splittedLine[1],"ADD")) type = ADD;
    else if(!strcmp(splittedLine[1],"CHA")) type = CHA;
    else return -1;

    char* name=splittedLine[2];
    if(strlen(name)<1) return -1;

    char* value=NULL;
    if(type!=DEL){
        value=splittedLine[3];
        if(strlen(value)<1) return -1;
    }

    char* host=NULL;
    if(type!=DEL) host=splittedLine[4];
    else host=splittedLine[3];

    struct headersRule* rule = (struct headersRule*)malloc(1*sizeof(struct headersRule));
    rule->type=type;

    if(value!=NULL) rule->value = (char*)malloc((strlen(value)+1)*sizeof(char));
    if(host!=NULL) rule->hostNamePattern = (char*)malloc((strlen(host)+1)*sizeof(char));
    rule->namePattern = (char*)malloc((strlen(name)+1)*sizeof(char));

    strcpy(rule->value,value);
    strcpy(rule->hostNamePattern,host);
    strcpy(rule->namePattern,name);

    regex_t *nameReg=(regex_t*)malloc(sizeof(regex_t));
    regex_t *hostReg=NULL;
    if(regcomp(nameReg,rule->namePattern,0)){
        free(rule);
        fprintf(stderr,"Unable to compile regex: %s\n",name);
        return -1;
    }
    if(host!=NULL) {
        hostReg=(regex_t*)malloc(sizeof(regex_t));
        if (regcomp(hostReg, rule->hostNamePattern, 0)) {
            free(rule);
            fprintf(stderr, "Unable to compile regex: %s\n", host);
            return -1;
        }
    }
    rule->hostNameRegex=hostReg;
    rule->nameRegex=nameReg;

    switch (headerCookie){
        case 0:
            if(config->headerRulesNumber>=headerRuleMalloc){
                headerRuleMalloc++;
                config->header=realloc(config->header,headerRuleMalloc*sizeof(struct blockRule));
            }
            config->header[config->headerRulesNumber]=*rule;
            config->headerRulesNumber++;
            break;
        case 1:
            if(config->cookieRulesNumber>=cookieRuleMalloc){
                cookieRuleMalloc++;
                config->cookie=realloc(config->cookie,cookieRuleMalloc*sizeof(struct blockRule));
            }
            config->cookie[config->cookieRulesNumber]=*rule;
            config->cookieRulesNumber++;
            break;
        default:
            free(rule);
            exit(1);
    }
    return 0;
}
