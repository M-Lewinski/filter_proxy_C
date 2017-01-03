#include "stdio.h"
#include "source/config.h"

int main(int argc, char** argv){
    puts("Starting filter proxy");
    struct configStruct* config = loadConfig();
    if(config == NULL){
        fprintf(stderr, "Loading file FAILED");
        return 1;
    }

    //TODO START PROXY

    freeConfig(config);
    return 0;
}