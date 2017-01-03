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
};

void freeConfig(struct configStruct* config){
    //TODO FREE CONFIG FILE

};

int parseLine(struct configStruct* config, char* line, ssize_t lineLen){
    if(line[lineLen-1]=='\n') line[lineLen-1]='\0'; //Removing new line
    while(line[0]=='\t' || line[0]==' ') line=line+1; //Removing whitespaces
    if(line[0]=='#' || line[0]=='\0') return 0; //Skipping empty lines and comments

    int maxPartCount=10, partCount=1;
    char** splittedLine = (char**)malloc(maxPartCount*sizeof(char*));
    char*lineCpy=(char*)malloc((strlen(line)+1)*sizeof(char));
    strcpy(lineCpy, line);
    splittedLine[0]=lineCpy;
    for(char* splitPointer = lineCpy+1;splitPointer[0]!='\0';splitPointer++) { //LINE SPLITTING
        if(maxPartCount<partCount){
            maxPartCount+=1;
            realloc(splittedLine,maxPartCount*sizeof(char*));
        }
        if ((splitPointer[0] != ' ' || splitPointer[0] != '\t') &&
            ((splitPointer - 1)[0] == ' ' || (splitPointer - 1)[0] == '\t')) {
            splittedLine[partCount] = splitPointer;
            (splitPointer - 1)[0] = '\0';
            partCount++;
        }
    }

    for(int i=0;i<partCount;i++)puts(splittedLine[i]);


    //TODO PARSE SINGLE LINE
    //TODO REALLOC MEMORY IF NEEDED
    free(splittedLine);
    free(lineCpy);
    return -1;
};