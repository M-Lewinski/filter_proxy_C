#include "stdio.h"
#include "source/config.h"

int main(int argc, char** argv){
    puts("Starting filter proxy");
    struct configStruct* config = loadConfig();
    //TODO LOAD CONFIG FILE

    freeConfig(config);
    return 0;
}