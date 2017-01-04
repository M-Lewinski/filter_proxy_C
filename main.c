#include "stdio.h"
#include "source/config.h"
#include "source/proxy.h"

int main(int argc, char** argv){
    if(argc<2){
        fprintf(stderr,"Not enough parameters");
        return 1;
    }
    struct configStruct* config;
    if(argc==3) config = loadConfigWithPath(argv[2]);
    else config = loadConfig();
    if(config == NULL){
        fprintf(stderr, "Loading file FAILED");
        return 1;
    }

    puts("Starting filter proxy");
    startProxyServer(argv[1],config);


    freeConfig(config);
    return 0;
}