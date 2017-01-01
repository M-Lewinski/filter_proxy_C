#include "config.h"

struct configStruct* loadConfig(){
    return loadConfigWithPath("../conf.ini");
};

struct configStruct* loadConfigWithPath(char* filePath){
    return NULL;
};

void freeConfig(struct configStruct* config){

};